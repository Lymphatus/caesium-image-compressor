use crate::config::PresetStore;
use crate::engine::{
    dry_run_batch, preview_file, run_batch, BatchReport, ItemStatus, JobReceipt, OutputTarget,
};
use crate::error::{CaesiumCliError, ErrorCode};
use crate::inspect::inspect_path;
use crate::options::{CompressionOptions, ImportFilters, DEFAULT_MAX_INPUT_BYTES};
use crate::output::capabilities_payload;
use crate::scanner::{common_root_folder, scan_inputs, ScanOptions};
use crate::SCHEMA_VERSION;
use serde_json::{json, Value};
use std::collections::HashSet;
use std::io::{self, BufRead, Write};
use std::path::PathBuf;

/// Stateful NDJSON agent session mirroring the GUI/TUI workflow programmatically.
/// Agents communicate by sending one JSON request per line on stdin and reading
/// one structured JSON response per line on stdout.
pub struct AgentSession {
    pub queue: Vec<PathBuf>,
    pub options: CompressionOptions,
    pub output_target: Option<OutputTarget>,
    pub last_report: Option<BatchReport>,
    pub presets: PresetStore,
    pub should_quit: bool,
}

impl Default for AgentSession {
    fn default() -> Self {
        Self::new()
    }
}

impl AgentSession {
    pub fn new() -> Self {
        Self {
            queue: Vec::new(),
            options: CompressionOptions::default(),
            output_target: None,
            last_report: None,
            presets: PresetStore::default_store(),
            should_quit: false,
        }
    }

    /// Runs the interactive NDJSON session over stdin/stdout until `quit` or EOF.
    pub fn run_stdio(&mut self) -> io::Result<()> {
        let stdin = io::stdin();
        let mut stdout = io::stdout().lock();

        // Emit ready handshake event so agents know session is live
        let ready = json!({
            "event": "ready",
            "schema_version": SCHEMA_VERSION,
            "capabilities": capabilities_payload()
        });
        writeln!(
            stdout,
            "{}",
            serde_json::to_string(&ready).unwrap_or_default()
        )?;
        stdout.flush()?;

        for line in stdin.lock().lines() {
            let line = line?;
            let trimmed = line.trim();
            if trimmed.is_empty() {
                continue;
            }
            let response = self.handle_line(trimmed);
            writeln!(
                stdout,
                "{}",
                serde_json::to_string(&response).unwrap_or_default()
            )?;
            stdout.flush()?;
            if self.should_quit {
                break;
            }
        }
        Ok(())
    }

    pub fn handle_line(&mut self, line: &str) -> Value {
        match serde_json::from_str::<Value>(line) {
            Ok(val) => self.handle_json(val),
            Err(e) => ok_or_err(
                Value::Null,
                "parse",
                Err(CaesiumCliError::new(
                    ErrorCode::ProtocolError,
                    format!("Malformed JSON request: {e}"),
                )),
            ),
        }
    }

    pub fn handle_json(&mut self, req: Value) -> Value {
        let id = req.get("id").cloned().unwrap_or(Value::Null);
        let op = match req.get("op").and_then(Value::as_str) {
            Some(op) => op.to_string(),
            None => {
                return ok_or_err(
                    id,
                    "unknown",
                    Err(CaesiumCliError::new(
                        ErrorCode::ProtocolError,
                        "Missing required string field 'op'",
                    )
                    .with_hint(
                        "Supported ops: capabilities, add, list, remove, clear, get_options, set_options, apply_preset, list_presets, inspect, preview, dry_run, compress, retry_failed, status, quit",
                    )),
                );
            }
        };

        let result = match op.as_str() {
            "capabilities" => Ok(capabilities_payload()),
            "add" => self.op_add(&req),
            "list" => Ok(json!({
                "queue_len": self.queue.len(),
                "root_folder": common_root_folder(&self.queue),
                "files": self.queue,
            })),
            "remove" => self.op_remove(&req),
            "clear" => {
                let removed = self.queue.len();
                self.queue.clear();
                Ok(json!({ "removed": removed, "queue_len": 0 }))
            }
            "get_options" => Ok(json!({
                "options": self.options,
                "warnings": self.options.warnings(),
            })),
            "set_options" => self.op_set_options(&req),
            "apply_preset" => self.op_apply_preset(&req),
            "list_presets" => self
                .presets
                .list()
                .map(|presets| json!({ "presets": presets })),
            "inspect" => self.op_inspect(&req),
            "preview" => self.op_preview(&req),
            "dry_run" => self.op_dry_run(&req),
            "compress" => self.op_compress(&req),
            "retry_failed" => self.op_retry_failed(&req),
            "status" => Ok(json!({
                "queue_len": self.queue.len(),
                "output_target": self.output_target,
                "options": self.options,
                "last_summary": self.last_report.as_ref().map(|r| &r.summary),
            })),
            "quit" | "exit" => {
                self.should_quit = true;
                Ok(json!({ "bye": true }))
            }
            other => Err(CaesiumCliError::new(
                ErrorCode::ProtocolError,
                format!("Unknown agent operation '{other}'"),
            )),
        };

        ok_or_err(id, &op, result)
    }

    fn op_add(&mut self, req: &Value) -> Result<Value, CaesiumCliError> {
        let paths: Vec<PathBuf> = req
            .get("paths")
            .and_then(Value::as_array)
            .ok_or_else(|| {
                CaesiumCliError::new(ErrorCode::InvalidInput, "Expected array field 'paths'")
            })?
            .iter()
            .filter_map(Value::as_str)
            .map(PathBuf::from)
            .collect();

        let recursive = req
            .get("recursive")
            .and_then(Value::as_bool)
            .unwrap_or(true);
        let filters: ImportFilters = req
            .get("filters")
            .cloned()
            .map(serde_json::from_value)
            .transpose()?
            .unwrap_or_default();
        let max_input_bytes = req
            .get("max_input_bytes")
            .and_then(Value::as_u64)
            .or(Some(DEFAULT_MAX_INPUT_BYTES));

        let scan = scan_inputs(
            &paths,
            &ScanOptions {
                recursive,
                filters,
                max_input_bytes,
            },
        )?;

        let mut existing: HashSet<PathBuf> = self.queue.iter().cloned().collect();
        let mut added = 0usize;
        for f in scan.files {
            if existing.insert(f.clone()) {
                self.queue.push(f);
                added += 1;
            }
        }

        Ok(json!({
            "added": added,
            "skipped": scan.skipped,
            "queue_len": self.queue.len(),
            "root_folder": common_root_folder(&self.queue),
        }))
    }

    fn op_remove(&mut self, req: &Value) -> Result<Value, CaesiumCliError> {
        let targets: HashSet<PathBuf> = req
            .get("paths")
            .and_then(Value::as_array)
            .ok_or_else(|| {
                CaesiumCliError::new(ErrorCode::InvalidInput, "Expected array field 'paths'")
            })?
            .iter()
            .filter_map(Value::as_str)
            .map(|s| {
                let p = PathBuf::from(s);
                p.canonicalize().unwrap_or(p)
            })
            .collect();

        let before = self.queue.len();
        self.queue.retain(|p| {
            let c = p.canonicalize().unwrap_or_else(|_| p.clone());
            !targets.contains(&c) && !targets.contains(p)
        });
        let removed = before - self.queue.len();
        Ok(json!({
            "removed": removed,
            "queue_len": self.queue.len(),
        }))
    }

    fn op_set_options(&mut self, req: &Value) -> Result<Value, CaesiumCliError> {
        let patch = req.get("options").ok_or_else(|| {
            CaesiumCliError::new(ErrorCode::InvalidOption, "Missing object field 'options'")
        })?;
        let mut merged = serde_json::to_value(&self.options)?;
        merge_json(&mut merged, patch);
        let updated: CompressionOptions = serde_json::from_value(merged)?;
        updated.validate()?;
        self.options = updated;
        Ok(json!({
            "options": self.options,
            "warnings": self.options.warnings(),
        }))
    }

    fn op_apply_preset(&mut self, req: &Value) -> Result<Value, CaesiumCliError> {
        let name = req.get("name").and_then(Value::as_str).ok_or_else(|| {
            CaesiumCliError::new(ErrorCode::InvalidOption, "Missing string field 'name'")
        })?;
        let preset = self.presets.get(name)?;
        self.options = preset.options.clone();
        Ok(json!({
            "preset": preset.name,
            "description": preset.description,
            "options": self.options,
            "warnings": self.options.warnings(),
        }))
    }

    fn op_inspect(&self, req: &Value) -> Result<Value, CaesiumCliError> {
        let paths: Vec<PathBuf> = match req.get("paths").and_then(Value::as_array) {
            Some(arr) => arr
                .iter()
                .filter_map(Value::as_str)
                .map(PathBuf::from)
                .collect(),
            None => self.queue.clone(),
        };
        let mut items = Vec::with_capacity(paths.len());
        for p in &paths {
            items.push(inspect_path(p)?);
        }
        Ok(json!({ "items": items }))
    }

    fn op_preview(&self, req: &Value) -> Result<Value, CaesiumCliError> {
        let paths: Vec<PathBuf> = match req.get("paths").and_then(Value::as_array) {
            Some(arr) => arr
                .iter()
                .filter_map(Value::as_str)
                .map(PathBuf::from)
                .collect(),
            None => self.queue.clone(),
        };
        if paths.is_empty() {
            return Err(CaesiumCliError::new(
                ErrorCode::NoMatchingInputs,
                "Queue is empty; pass paths or call 'add' first",
            ));
        }
        let mut items = Vec::with_capacity(paths.len());
        for p in &paths {
            items.push(preview_file(p, &self.options)?);
        }
        Ok(json!({ "items": items }))
    }

    fn resolve_target(&self, req: &Value) -> Result<OutputTarget, CaesiumCliError> {
        if let Some(out) = req.get("output").and_then(Value::as_str) {
            let p = PathBuf::from(out);
            if self.queue.len() == 1
                && !p.is_dir()
                && p.extension().is_some()
                && !out.ends_with(std::path::MAIN_SEPARATOR)
            {
                return Ok(OutputTarget::SingleFile(p));
            }
            return Ok(OutputTarget::Directory(p));
        }
        if req
            .get("same_folder")
            .and_then(Value::as_bool)
            .unwrap_or(self.options.same_folder_as_input)
        {
            return Ok(OutputTarget::SameFolder);
        }
        if let Some(t) = &self.output_target {
            return Ok(t.clone());
        }
        Err(CaesiumCliError::new(
            ErrorCode::InvalidOption,
            "Output destination required (provide 'output' or set 'same_folder': true)",
        ))
    }

    fn op_dry_run(&self, req: &Value) -> Result<Value, CaesiumCliError> {
        let target = self.resolve_target(req)?;
        let plan = dry_run_batch(&self.queue, target, &self.options, None)?;
        Ok(serde_json::to_value(plan)?)
    }

    fn op_compress(&mut self, req: &Value) -> Result<Value, CaesiumCliError> {
        let target = self.resolve_target(req)?;
        let threads = req.get("threads").and_then(Value::as_u64).unwrap_or(0) as usize;
        let report = run_batch(&self.queue, target.clone(), &self.options, threads, None)?;
        if let Some(receipt_path) = req.get("receipt").and_then(Value::as_str) {
            let receipt = JobReceipt::from_report(&report, target.clone(), &self.options);
            receipt.save(&PathBuf::from(receipt_path))?;
        }
        self.output_target = Some(target);
        self.last_report = Some(report.clone());
        Ok(serde_json::to_value(report)?)
    }

    fn op_retry_failed(&mut self, req: &Value) -> Result<Value, CaesiumCliError> {
        let last = self.last_report.as_ref().ok_or_else(|| {
            CaesiumCliError::new(
                ErrorCode::InvalidInput,
                "No previous compression run in this session",
            )
        })?;
        let failed: Vec<PathBuf> = last
            .items
            .iter()
            .filter(|i| matches!(i.status, ItemStatus::Error | ItemStatus::Canceled))
            .map(|i| i.input_path.clone())
            .collect();
        if failed.is_empty() {
            return Ok(json!({
                "retried": 0,
                "message": "No failed items from previous run"
            }));
        }
        let target = self.resolve_target(req)?;
        let threads = req.get("threads").and_then(Value::as_u64).unwrap_or(0) as usize;
        let report = run_batch(
            &failed,
            target,
            &self.options,
            threads,
            Some(last.root_folder.clone()),
        )?;
        self.last_report = Some(report.clone());
        Ok(serde_json::to_value(report)?)
    }
}

fn ok_or_err(id: Value, op: &str, result: Result<Value, CaesiumCliError>) -> Value {
    match result {
        Ok(res) => json!({
            "id": id,
            "op": op,
            "ok": true,
            "schema_version": SCHEMA_VERSION,
            "result": res,
        }),
        Err(err) => json!({
            "id": id,
            "op": op,
            "ok": false,
            "schema_version": SCHEMA_VERSION,
            "error": err,
        }),
    }
}

pub fn merge_json(base: &mut Value, patch: &Value) {
    match (base, patch) {
        (Value::Object(base_map), Value::Object(patch_map)) => {
            for (k, v) in patch_map {
                if let Some(existing) = base_map.get_mut(k) {
                    merge_json(existing, v);
                } else {
                    base_map.insert(k.clone(), v.clone());
                }
            }
        }
        (base_slot, patch_val) => {
            *base_slot = patch_val.clone();
        }
    }
}
