use crate::agent::merge_json;
use crate::config::PresetStore;
use crate::engine::{
    dry_run_batch, preview_file, retry_from_receipt, run_batch, JobReceipt, OutputTarget,
};
use crate::error::{CaesiumCliError, CliResult, ErrorCode};
use crate::inspect::inspect_path;
use crate::options::{
    BatchJobSpec, CompressionMode, CompressionOptions, ImportFilters, MaxOutputSize, OutputFormat,
    ResizeOptions, DEFAULT_MAX_INPUT_BYTES,
};
use crate::output::{capabilities_payload, schema_for_target};
use crate::scanner::{scan_inputs, ScanOptions};
use crate::{APP_NAME, APP_VERSION};
use serde_json::{json, Value};
use std::io::{self, BufRead, Read, Write};
use std::path::PathBuf;

const MCP_PROTOCOL_VERSION: &str = "2024-11-05";

/// Native Model Context Protocol (MCP) JSON-RPC 2.0 stdio server.
/// Supports both newline-delimited JSON-RPC (standard MCP stdio) and `Content-Length` framing.
pub struct McpServer {
    presets: PresetStore,
}

impl Default for McpServer {
    fn default() -> Self {
        Self::new()
    }
}

impl McpServer {
    pub fn new() -> Self {
        Self {
            presets: PresetStore::default_store(),
        }
    }

    pub fn run_stdio(&mut self) -> io::Result<()> {
        let stdin = io::stdin();
        let mut reader = stdin.lock();
        let mut stdout = io::stdout().lock();

        loop {
            let mut first_line = String::new();
            let n = reader.read_line(&mut first_line)?;
            if n == 0 {
                break;
            }
            let trimmed = first_line.trim();
            if trimmed.is_empty() {
                continue;
            }

            let (msg_str, use_content_length) =
                if trimmed.to_ascii_lowercase().starts_with("content-length:") {
                    let len: usize = trimmed
                        .split_once(':')
                        .and_then(|(_, v)| v.trim().parse().ok())
                        .unwrap_or(0);
                    // Consume remaining headers until empty line
                    loop {
                        let mut hdr = String::new();
                        let r = reader.read_line(&mut hdr)?;
                        if r == 0 || hdr.trim().is_empty() {
                            break;
                        }
                    }
                    let mut body = vec![0u8; len];
                    reader.read_exact(&mut body)?;
                    (String::from_utf8_lossy(&body).to_string(), true)
                } else {
                    (trimmed.to_string(), false)
                };

            let parsed: Result<Value, _> = serde_json::from_str(&msg_str);
            let response = match parsed {
                Ok(val) => self.handle_message(val),
                Err(e) => Some(json!({
                    "jsonrpc": "2.0",
                    "id": Value::Null,
                    "error": {
                        "code": -32700,
                        "message": format!("Parse error: {e}")
                    }
                })),
            };

            if let Some(resp) = response {
                let encoded = serde_json::to_string(&resp).unwrap_or_default();
                let write_res = if use_content_length {
                    write!(
                        stdout,
                        "Content-Length: {}\r\n\r\n{}",
                        encoded.len(),
                        encoded
                    )
                    .and_then(|_| stdout.flush())
                } else {
                    writeln!(stdout, "{encoded}").and_then(|_| stdout.flush())
                };
                if let Err(e) = write_res {
                    if e.kind() == io::ErrorKind::BrokenPipe {
                        break;
                    }
                    return Err(e);
                }
            }
        }

        Ok(())
    }

    pub fn handle_message(&mut self, msg: Value) -> Option<Value> {
        let id = msg.get("id").cloned();
        let method = msg.get("method").and_then(Value::as_str).unwrap_or("");
        let params = msg.get("params").cloned().unwrap_or_else(|| json!({}));

        // Notifications have no id and require no response
        if id.is_none() && method.starts_with("notifications/") {
            return None;
        }
        let id = id.unwrap_or(Value::Null);

        match method {
            "initialize" => Some(json!({
                "jsonrpc": "2.0",
                "id": id,
                "result": {
                    "protocolVersion": MCP_PROTOCOL_VERSION,
                    "capabilities": {
                        "tools": { "listChanged": false },
                        "resources": { "subscribe": false, "listChanged": false },
                        "prompts": { "listChanged": false }
                    },
                    "serverInfo": {
                        "name": APP_NAME,
                        "version": APP_VERSION
                    },
                    "instructions": "Caesium Image Compressor MCP server. Inspect images before compressing, preview savings when tuning quality, and prefer non-destructive output directories unless overwrite is explicitly intended."
                }
            })),
            "ping" => Some(json!({
                "jsonrpc": "2.0",
                "id": id,
                "result": {}
            })),
            "tools/list" => Some(json!({
                "jsonrpc": "2.0",
                "id": id,
                "result": {
                    "tools": self.tool_definitions()
                }
            })),
            "tools/call" => {
                let tool_name = params.get("name").and_then(Value::as_str).unwrap_or("");
                let args = params
                    .get("arguments")
                    .cloned()
                    .unwrap_or_else(|| json!({}));
                let call_result = self.call_tool(tool_name, &args);
                Some(match call_result {
                    Ok(payload) => json!({
                        "jsonrpc": "2.0",
                        "id": id,
                        "result": {
                            "content": [{
                                "type": "text",
                                "text": serde_json::to_string_pretty(&payload).unwrap_or_default()
                            }],
                            "structuredContent": payload,
                            "isError": false
                        }
                    }),
                    Err(err) => {
                        let err_val = json!({ "error": err });
                        json!({
                            "jsonrpc": "2.0",
                            "id": id,
                            "result": {
                                "content": [{
                                    "type": "text",
                                    "text": serde_json::to_string_pretty(&err_val).unwrap_or_default()
                                }],
                                "structuredContent": err_val,
                                "isError": true
                            }
                        })
                    }
                })
            }
            "resources/list" => Some(json!({
                "jsonrpc": "2.0",
                "id": id,
                "result": {
                    "resources": [
                        {
                            "uri": "caesium://capabilities",
                            "name": "Caesium Capabilities",
                            "description": "Supported codecs, formats, resize modes, exit codes, and protocols",
                            "mimeType": "application/json"
                        },
                        {
                            "uri": "caesium://presets",
                            "name": "Compression Presets",
                            "description": "Built-in and user-saved Caesium compression presets",
                            "mimeType": "application/json"
                        },
                        {
                            "uri": "caesium://schema/options",
                            "name": "CompressionOptions JSON Schema",
                            "description": "Draft 2020-12 JSON Schema for all Caesium compression options",
                            "mimeType": "application/schema+json"
                        },
                        {
                            "uri": "caesium://schema/batch",
                            "name": "BatchJobSpec JSON Schema",
                            "description": "Draft 2020-12 JSON Schema for multi-task batch job specifications",
                            "mimeType": "application/schema+json"
                        }
                    ]
                }
            })),
            "resources/read" => {
                let uri = params.get("uri").and_then(Value::as_str).unwrap_or("");
                match self.read_resource(uri) {
                    Ok((mime, text)) => Some(json!({
                        "jsonrpc": "2.0",
                        "id": id,
                        "result": {
                            "contents": [{
                                "uri": uri,
                                "mimeType": mime,
                                "text": text
                            }]
                        }
                    })),
                    Err(err) => Some(json!({
                        "jsonrpc": "2.0",
                        "id": id,
                        "error": {
                            "code": -32602,
                            "message": err.message
                        }
                    })),
                }
            }
            "prompts/list" => Some(json!({
                "jsonrpc": "2.0",
                "id": id,
                "result": {
                    "prompts": [
                        {
                            "name": "optimize-for-web",
                            "description": "Inspect and optimize images for fast web delivery (WebP + responsive sizing)",
                            "arguments": [
                                { "name": "input_dir", "description": "Source folder containing images", "required": true },
                                { "name": "output_dir", "description": "Destination folder", "required": true }
                            ]
                        },
                        {
                            "name": "fit-under-size-budget",
                            "description": "Compress images so every output stays under a strict byte budget",
                            "arguments": [
                                { "name": "inputs", "description": "File or directory paths", "required": true },
                                { "name": "max_size", "description": "Target ceiling e.g. 250KB or 1MB", "required": true }
                            ]
                        },
                        {
                            "name": "batch-convert-assets",
                            "description": "Generate multiple format and resolution variants in a single declarative batch",
                            "arguments": [
                                { "name": "input_dir", "description": "Source asset folder", "required": true }
                            ]
                        }
                    ]
                }
            })),
            "prompts/get" => {
                let name = params.get("name").and_then(Value::as_str).unwrap_or("");
                let args = params
                    .get("arguments")
                    .cloned()
                    .unwrap_or_else(|| json!({}));
                match self.get_prompt(name, &args) {
                    Ok(prompt) => Some(json!({
                        "jsonrpc": "2.0",
                        "id": id,
                        "result": prompt
                    })),
                    Err(err) => Some(json!({
                        "jsonrpc": "2.0",
                        "id": id,
                        "error": { "code": -32602, "message": err.message }
                    })),
                }
            }
            _ => Some(json!({
                "jsonrpc": "2.0",
                "id": id,
                "error": {
                    "code": -32601,
                    "message": format!("Method not found: {method}")
                }
            })),
        }
    }

    fn tool_definitions(&self) -> Value {
        json!([
            {
                "name": "caesium_inspect",
                "description": "Inspect one or more image files or directories (format, MIME, dimensions, EXIF orientation, effective dimensions, byte size, EXIF/ICC presence, SHA-256) without modifying anything.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "paths": { "type": "array", "items": { "type": "string" }, "description": "File or directory paths to inspect" },
                        "recursive": { "type": "boolean", "default": true }
                    },
                    "required": ["paths"]
                }
            },
            {
                "name": "caesium_preview",
                "description": "Run in-memory compression preview to measure exact output size, byte savings, and dimensions without writing any files.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "paths": { "type": "array", "items": { "type": "string" } },
                        "preset": { "type": "string" },
                        "quality": { "type": "integer", "minimum": 0, "maximum": 100 },
                        "format": { "type": "string", "enum": ["same", "jpg", "png", "webp", "tiff", "gif"] },
                        "max_size": { "type": "string", "description": "Target ceiling e.g. 250KB, 1MB, 50%" },
                        "resize": { "type": "string", "description": "Shorthand e.g. 1920x1080, 50%, long:1200, w:800" },
                        "lossless": { "type": "boolean" },
                        "options": { "type": "object", "description": "Full CompressionOptions overrides" }
                    },
                    "required": ["paths"]
                }
            },
            {
                "name": "caesium_compress",
                "description": "Compress images or directories with full Caesium GUI parity (quality/size mode, JPEG/PNG/WebP/TIFF/GIF tuning, EXIF-aware resize, format conversion, directory structure, metadata/dates preservation, filters, dry-run, receipts).",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "inputs": { "type": "array", "items": { "type": "string" } },
                        "output": { "type": "string", "description": "Output directory or single file path" },
                        "same_folder": { "type": "boolean", "default": false },
                        "suffix": { "type": "string" },
                        "overwrite": { "type": "boolean", "default": false },
                        "dry_run": { "type": "boolean", "default": false },
                        "preset": { "type": "string" },
                        "quality": { "type": "integer", "minimum": 0, "maximum": 100 },
                        "jpeg_quality": { "type": "integer", "minimum": 0, "maximum": 100 },
                        "png_quality": { "type": "integer", "minimum": 0, "maximum": 100 },
                        "webp_quality": { "type": "integer", "minimum": 0, "maximum": 100 },
                        "gif_quality": { "type": "integer", "minimum": 0, "maximum": 100 },
                        "lossless": { "type": "boolean" },
                        "keep_metadata": { "type": "boolean" },
                        "keep_structure": { "type": "boolean" },
                        "skip_if_bigger": { "type": "boolean" },
                        "format": { "type": "string", "enum": ["same", "jpg", "png", "webp", "tiff", "gif"] },
                        "max_size": { "type": "string" },
                        "resize": { "type": "string" },
                        "recursive": { "type": "boolean", "default": true },
                        "filters": { "type": "object" },
                        "threads": { "type": "integer", "minimum": 0 },
                        "receipt": { "type": "string", "description": "Path to save job receipt JSON for later retry" },
                        "options": { "type": "object", "description": "Additional CompressionOptions fields" }
                    },
                    "required": ["inputs"]
                }
            },
            {
                "name": "caesium_convert",
                "description": "Convert images between JPG, PNG, WebP, TIFF, and GIF with optional compression and resizing.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "inputs": { "type": "array", "items": { "type": "string" } },
                        "to": { "type": "string", "enum": ["jpg", "png", "webp", "tiff", "gif"] },
                        "output": { "type": "string" },
                        "same_folder": { "type": "boolean", "default": false },
                        "quality": { "type": "integer", "minimum": 0, "maximum": 100 },
                        "resize": { "type": "string" },
                        "skip_if_bigger": { "type": "boolean", "default": false },
                        "overwrite": { "type": "boolean", "default": false },
                        "dry_run": { "type": "boolean", "default": false }
                    },
                    "required": ["inputs", "to"]
                }
            },
            {
                "name": "caesium_batch",
                "description": "Execute a multi-task declarative BatchJobSpec (multiple independent output formats/resolutions/directories in one call).",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "spec": { "type": "object", "description": "BatchJobSpec object with 'tasks' array" }
                    },
                    "required": ["spec"]
                }
            },
            {
                "name": "caesium_presets",
                "description": "List, get, save, or delete Caesium compression presets.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "action": { "type": "string", "enum": ["list", "get", "save", "delete"] },
                        "name": { "type": "string" },
                        "description": { "type": "string" },
                        "options": { "type": "object" }
                    },
                    "required": ["action"]
                }
            },
            {
                "name": "caesium_retry",
                "description": "Retry only failed items from a saved Caesium job receipt JSON file.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "receipt": { "type": "string" },
                        "threads": { "type": "integer", "minimum": 0 }
                    },
                    "required": ["receipt"]
                }
            }
        ])
    }

    fn call_tool(&mut self, name: &str, args: &Value) -> CliResult<Value> {
        match name {
            "caesium_inspect" => {
                let paths = extract_paths(args, "paths")?;
                let recursive = args
                    .get("recursive")
                    .and_then(Value::as_bool)
                    .unwrap_or(true);
                let scan = scan_inputs(
                    &paths,
                    &ScanOptions {
                        recursive,
                        filters: ImportFilters::default(),
                        max_input_bytes: None,
                    },
                )?;
                let mut items = Vec::with_capacity(scan.files.len());
                for f in &scan.files {
                    items.push(inspect_path(f)?);
                }
                Ok(json!({ "items": items, "skipped": scan.skipped }))
            }
            "caesium_preview" => {
                let paths = extract_paths(args, "paths")?;
                let opts = self.build_options_from_args(args)?;
                let scan = scan_inputs(&paths, &ScanOptions::default())?;
                if scan.files.is_empty() {
                    return Err(CaesiumCliError::new(
                        ErrorCode::NoMatchingInputs,
                        "No matching images found for preview",
                    ));
                }
                let mut items = Vec::with_capacity(scan.files.len());
                for f in &scan.files {
                    items.push(preview_file(f, &opts)?);
                }
                Ok(json!({ "items": items }))
            }
            "caesium_compress" => self.exec_compress_like(args, None),
            "caesium_convert" => {
                let to = args.get("to").and_then(Value::as_str).ok_or_else(|| {
                    CaesiumCliError::new(ErrorCode::InvalidOption, "Missing required 'to' format")
                })?;
                let fmt: OutputFormat =
                    serde_json::from_value(Value::String(to.to_ascii_lowercase()))?;
                self.exec_compress_like(args, Some(fmt))
            }
            "caesium_batch" => {
                let spec_val = args.get("spec").cloned().ok_or_else(|| {
                    CaesiumCliError::new(ErrorCode::InvalidOption, "Missing 'spec' object")
                })?;
                let spec: BatchJobSpec = serde_json::from_value(spec_val)?;
                execute_batch_spec(&spec, &self.presets)
            }
            "caesium_presets" => {
                let action = args.get("action").and_then(Value::as_str).unwrap_or("list");
                match action {
                    "list" => Ok(json!({ "presets": self.presets.list()? })),
                    "get" => {
                        let name = args.get("name").and_then(Value::as_str).ok_or_else(|| {
                            CaesiumCliError::new(ErrorCode::InvalidOption, "Missing preset 'name'")
                        })?;
                        Ok(serde_json::to_value(self.presets.get(name)?)?)
                    }
                    "save" => {
                        let name = args.get("name").and_then(Value::as_str).ok_or_else(|| {
                            CaesiumCliError::new(ErrorCode::InvalidOption, "Missing preset 'name'")
                        })?;
                        let desc = args
                            .get("description")
                            .and_then(Value::as_str)
                            .map(String::from);
                        let opts = self.build_options_from_args(args)?;
                        let entry = self.presets.save(name, desc, &opts)?;
                        Ok(serde_json::to_value(entry)?)
                    }
                    "delete" => {
                        let name = args.get("name").and_then(Value::as_str).ok_or_else(|| {
                            CaesiumCliError::new(ErrorCode::InvalidOption, "Missing preset 'name'")
                        })?;
                        self.presets.delete(name)?;
                        Ok(json!({ "deleted": name }))
                    }
                    other => Err(CaesiumCliError::new(
                        ErrorCode::InvalidOption,
                        format!("Unknown preset action '{other}'"),
                    )),
                }
            }
            "caesium_retry" => {
                let receipt = args.get("receipt").and_then(Value::as_str).ok_or_else(|| {
                    CaesiumCliError::new(ErrorCode::InvalidOption, "Missing 'receipt' path")
                })?;
                let threads = args.get("threads").and_then(Value::as_u64).unwrap_or(0) as usize;
                let report = retry_from_receipt(&PathBuf::from(receipt), threads)?;
                Ok(serde_json::to_value(report)?)
            }
            other => Err(CaesiumCliError::new(
                ErrorCode::ProtocolError,
                format!("Unknown tool '{other}'"),
            )),
        }
    }

    fn exec_compress_like(
        &self,
        args: &Value,
        forced_format: Option<OutputFormat>,
    ) -> CliResult<Value> {
        let inputs = extract_paths(args, "inputs")?;
        let mut opts = self.build_options_from_args(args)?;
        if let Some(fmt) = forced_format {
            opts.format = fmt;
            if args.get("skip_if_bigger").is_none() {
                opts.skip_if_bigger = false;
            }
        }

        let recursive = args
            .get("recursive")
            .and_then(Value::as_bool)
            .unwrap_or(true);
        let filters: ImportFilters = args
            .get("filters")
            .cloned()
            .map(serde_json::from_value)
            .transpose()?
            .unwrap_or_default();

        let scan = scan_inputs(
            &inputs,
            &ScanOptions {
                recursive,
                filters,
                max_input_bytes: Some(DEFAULT_MAX_INPUT_BYTES),
            },
        )?;

        let target = if let Some(out) = args.get("output").and_then(Value::as_str) {
            let p = PathBuf::from(out);
            if scan.files.len() == 1
                && !p.is_dir()
                && p.extension().is_some()
                && !out.ends_with(std::path::MAIN_SEPARATOR)
            {
                OutputTarget::SingleFile(p)
            } else {
                OutputTarget::Directory(p)
            }
        } else if args
            .get("same_folder")
            .and_then(Value::as_bool)
            .unwrap_or(opts.same_folder_as_input)
        {
            OutputTarget::SameFolder
        } else {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                "Provide 'output' directory/file or set 'same_folder': true",
            ));
        };

        if args
            .get("dry_run")
            .and_then(Value::as_bool)
            .unwrap_or(false)
        {
            let plan = dry_run_batch(&scan.files, target, &opts, Some(scan.root_folder))?;
            return Ok(serde_json::to_value(plan)?);
        }

        let threads = args.get("threads").and_then(Value::as_u64).unwrap_or(0) as usize;
        let report = run_batch(
            &scan.files,
            target.clone(),
            &opts,
            threads,
            Some(scan.root_folder),
        )?;

        if let Some(receipt_path) = args.get("receipt").and_then(Value::as_str) {
            let receipt = JobReceipt::from_report(&report, target, &opts);
            receipt.save(&PathBuf::from(receipt_path))?;
        }

        Ok(serde_json::to_value(report)?)
    }

    fn build_options_from_args(&self, args: &Value) -> CliResult<CompressionOptions> {
        let mut opts = if let Some(preset_name) = args.get("preset").and_then(Value::as_str) {
            self.presets.get(preset_name)?.options
        } else {
            CompressionOptions::default()
        };

        if let Some(raw_opts) = args.get("options") {
            let mut base = serde_json::to_value(&opts)?;
            merge_json(&mut base, raw_opts);
            opts = serde_json::from_value(base)?;
        }

        if let Some(q) = args.get("quality").and_then(Value::as_u64) {
            opts.apply_universal_quality(q as u32);
        }
        if let Some(q) = args.get("jpeg_quality").and_then(Value::as_u64) {
            opts.jpeg_quality = q as u32;
        }
        if let Some(q) = args.get("png_quality").and_then(Value::as_u64) {
            opts.png_quality = q as u32;
        }
        if let Some(q) = args.get("webp_quality").and_then(Value::as_u64) {
            opts.webp_quality = q as u32;
        }
        if let Some(q) = args.get("gif_quality").and_then(Value::as_u64) {
            opts.gif_quality = q as u32;
        }
        if let Some(v) = args.get("lossless").and_then(Value::as_bool) {
            opts.lossless = v;
        }
        if let Some(v) = args.get("keep_metadata").and_then(Value::as_bool) {
            opts.keep_metadata = v;
        }
        if let Some(v) = args.get("keep_structure").and_then(Value::as_bool) {
            opts.keep_structure = v;
        }
        if let Some(v) = args.get("skip_if_bigger").and_then(Value::as_bool) {
            opts.skip_if_bigger = v;
        }
        if let Some(v) = args.get("overwrite").and_then(Value::as_bool) {
            opts.overwrite = v;
        }
        if let Some(v) = args.get("same_folder").and_then(Value::as_bool) {
            opts.same_folder_as_input = v;
        }
        if let Some(s) = args.get("suffix").and_then(Value::as_str) {
            opts.suffix = s.to_string();
        }
        if let Some(f) = args.get("format").and_then(Value::as_str) {
            opts.format = serde_json::from_value(Value::String(f.to_ascii_lowercase()))?;
        }
        if let Some(ms) = args.get("max_size").and_then(Value::as_str) {
            opts.mode = CompressionMode::Size;
            opts.max_output_size = MaxOutputSize::parse(ms)?;
        }
        if let Some(rs) = args.get("resize").and_then(Value::as_str) {
            opts.resize = ResizeOptions::parse_spec(rs)?;
        }

        opts.validate()?;
        Ok(opts)
    }

    fn read_resource(&self, uri: &str) -> CliResult<(&'static str, String)> {
        match uri {
            "caesium://capabilities" => Ok((
                "application/json",
                serde_json::to_string_pretty(&capabilities_payload())?,
            )),
            "caesium://presets" => Ok((
                "application/json",
                serde_json::to_string_pretty(&self.presets.list()?)?,
            )),
            "caesium://schema/options" => Ok((
                "application/schema+json",
                serde_json::to_string_pretty(&schema_for_target("options")?)?,
            )),
            "caesium://schema/batch" => Ok((
                "application/schema+json",
                serde_json::to_string_pretty(&schema_for_target("batch")?)?,
            )),
            other => Err(CaesiumCliError::new(
                ErrorCode::InvalidInput,
                format!("Unknown resource URI '{other}'"),
            )),
        }
    }

    fn get_prompt(&self, name: &str, args: &Value) -> CliResult<Value> {
        let msg = match name {
            "optimize-for-web" => {
                let inp = args
                    .get("input_dir")
                    .and_then(Value::as_str)
                    .unwrap_or("<input_dir>");
                let out = args
                    .get("output_dir")
                    .and_then(Value::as_str)
                    .unwrap_or("<output_dir>");
                format!(
                    "1. Call caesium_inspect on '{inp}' to see formats, dimensions, and byte sizes.\n2. Call caesium_preview with preset='web' on representative files to confirm visual quality and savings.\n3. Call caesium_compress with inputs=['{inp}'], output='{out}', preset='web', keep_structure=true."
                )
            }
            "fit-under-size-budget" => {
                let inp = args
                    .get("inputs")
                    .and_then(Value::as_str)
                    .unwrap_or("<inputs>");
                let max_size = args
                    .get("max_size")
                    .and_then(Value::as_str)
                    .unwrap_or("250KB");
                format!(
                    "1. Call caesium_inspect on '{inp}'.\n2. Call caesium_preview with max_size='{max_size}'.\n3. Call caesium_compress with max_size='{max_size}' to a dedicated output directory."
                )
            }
            "batch-convert-assets" => {
                let inp = args
                    .get("input_dir")
                    .and_then(Value::as_str)
                    .unwrap_or("<input_dir>");
                format!(
                    "Read caesium://schema/batch, then call caesium_batch with tasks producing WebP (@1x/@2x), JPEG fallback, and thumbnails from '{inp}'."
                )
            }
            other => {
                return Err(CaesiumCliError::new(
                    ErrorCode::InvalidInput,
                    format!("Unknown prompt '{other}'"),
                ));
            }
        };
        Ok(json!({
            "description": format!("Caesium workflow prompt: {name}"),
            "messages": [{
                "role": "user",
                "content": { "type": "text", "text": msg }
            }]
        }))
    }
}

fn extract_paths(args: &Value, field: &str) -> CliResult<Vec<PathBuf>> {
    let arr = args.get(field).and_then(Value::as_array).ok_or_else(|| {
        CaesiumCliError::new(
            ErrorCode::InvalidInput,
            format!("Missing required string array field '{field}'"),
        )
    })?;
    Ok(arr
        .iter()
        .filter_map(Value::as_str)
        .map(PathBuf::from)
        .collect())
}

/// Shared executor for `BatchJobSpec` used by both `caesium-cli batch` and MCP `caesium_batch`.
pub fn execute_batch_spec(spec: &BatchJobSpec, presets: &PresetStore) -> CliResult<Value> {
    let mut task_results = Vec::with_capacity(spec.tasks.len());
    let mut total_failed = 0usize;
    let threads = spec.threads.unwrap_or(0);

    for (idx, task) in spec.tasks.iter().enumerate() {
        let task_name = task
            .name
            .clone()
            .unwrap_or_else(|| format!("task-{}", idx + 1));

        let mut opts = if let Some(preset_name) = &task.preset {
            presets.get(preset_name)?.options
        } else {
            CompressionOptions::default()
        };
        // Overlay explicit task options over preset defaults
        if task.preset.is_some() {
            let task_json = serde_json::to_value(&task.options)?;
            let default_json = serde_json::to_value(CompressionOptions::default())?;
            let mut merged = serde_json::to_value(&opts)?;
            if let (Some(t_obj), Some(d_obj), Some(m_obj)) = (
                task_json.as_object(),
                default_json.as_object(),
                merged.as_object_mut(),
            ) {
                for (k, v) in t_obj {
                    if d_obj.get(k) != Some(v) {
                        m_obj.insert(k.clone(), v.clone());
                    }
                }
            }
            opts = serde_json::from_value(merged)?;
        } else {
            opts = task.options.clone();
        }
        opts.validate()?;

        let input_paths: Vec<PathBuf> = task.inputs.iter().map(PathBuf::from).collect();
        let scan = scan_inputs(
            &input_paths,
            &ScanOptions {
                recursive: task.recursive,
                filters: task.filters.clone(),
                max_input_bytes: Some(DEFAULT_MAX_INPUT_BYTES),
            },
        )?;

        let target = if let Some(out) = &task.output {
            let p = PathBuf::from(out);
            if scan.files.len() == 1
                && !p.is_dir()
                && p.extension().is_some()
                && !out.ends_with(std::path::MAIN_SEPARATOR)
            {
                OutputTarget::SingleFile(p)
            } else {
                OutputTarget::Directory(p)
            }
        } else if opts.same_folder_as_input {
            OutputTarget::SameFolder
        } else {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!(
                    "Task '{task_name}' requires 'output' or 'options.same_folder_as_input: true'"
                ),
            ));
        };

        if spec.dry_run {
            let plan = dry_run_batch(&scan.files, target, &opts, Some(scan.root_folder))?;
            task_results.push(json!({
                "name": task_name,
                "dry_run": true,
                "plan": plan,
            }));
        } else {
            let report = run_batch(&scan.files, target, &opts, threads, Some(scan.root_folder))?;
            total_failed += report.summary.failed;
            task_results.push(json!({
                "name": task_name,
                "dry_run": false,
                "report": report,
            }));
        }
    }

    Ok(json!({
        "tasks": task_results,
        "total_tasks": spec.tasks.len(),
        "total_failed": total_failed,
    }))
}
