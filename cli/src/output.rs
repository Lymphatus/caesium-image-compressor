use crate::config::PresetEntry;
use crate::engine::{BatchReport, DryRunPlan, ItemResult, ItemStatus, JobReceipt, PreviewResult};
use crate::error::{exit_codes, CaesiumCliError, CliResult, ErrorCode};
use crate::inspect::ImageInfo;
use crate::options::{format_human_size, BatchJobSpec, CompressionOptions};
use crate::{APP_NAME, APP_VERSION, SCHEMA_VERSION};
use clap::ValueEnum;
use schemars::schema_for;
use serde::Serialize;
use serde_json::{json, Value};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default, ValueEnum)]
pub enum OutputStyle {
    #[default]
    Human,
    Json,
    Ndjson,
    Quiet,
}

#[derive(Debug, Serialize)]
pub struct JsonEnvelope<T: Serialize> {
    pub ok: bool,
    pub schema_version: &'static str,
    pub command: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub result: Option<T>,
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub warnings: Vec<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub error: Option<CaesiumCliError>,
}

impl<T: Serialize> JsonEnvelope<T> {
    pub fn ok(command: impl Into<String>, result: T, warnings: Vec<String>) -> Self {
        Self {
            ok: true,
            schema_version: SCHEMA_VERSION,
            command: command.into(),
            result: Some(result),
            warnings,
            error: None,
        }
    }
}

pub fn error_envelope(command: impl Into<String>, err: &CaesiumCliError) -> Value {
    json!({
        "ok": false,
        "schema_version": SCHEMA_VERSION,
        "command": command.into(),
        "error": err,
    })
}

pub fn print_json<T: Serialize>(value: &T) {
    if let Ok(s) = serde_json::to_string_pretty(value) {
        println!("{s}");
    }
}

pub fn print_ndjson_line(value: &Value) {
    if let Ok(s) = serde_json::to_string(value) {
        println!("{s}");
    }
}

/// Complete machine-readable capability manifest for agents and tool orchestrators.
pub fn capabilities_payload() -> Value {
    json!({
        "app": APP_NAME,
        "version": APP_VERSION,
        "schema_version": SCHEMA_VERSION,
        "engine": {
            "name": "libcaesium",
            "version": "0.17.4",
            "codecs": ["mozjpeg", "oxipng", "lodepng", "imagequant", "zopfli", "webp", "gifsicle", "tiff"],
            "multithreaded": true,
            "in_memory_streaming": true
        },
        "formats": {
            "input": ["jpg", "jpeg", "png", "webp", "tiff", "tif", "gif"],
            "output": ["same", "jpg", "png", "webp", "tiff", "gif"]
        },
        "compression_modes": ["quality", "size"],
        "max_size_units": ["bytes", "kb", "mb", "%"],
        "resize_modes": [
            "none",
            "dimensions",
            "percentage",
            "short_edge",
            "long_edge",
            "fixed_width",
            "fixed_height"
        ],
        "protocols": {
            "cli": {
                "output_formats": ["human", "json", "ndjson", "quiet"],
                "stdin_stdout_pipe": true,
                "dry_run": true,
                "receipt_retry": true
            },
            "tui": {
                "command": "caesium-cli tui [PATHS...]",
                "features": [
                    "file_queue",
                    "compression_tab",
                    "resize_tab",
                    "output_tab",
                    "live_preview",
                    "presets",
                    "advanced_import_filters",
                    "multithreaded_progress",
                    "cancel_and_retry_failed",
                    "overwrite_guard"
                ]
            },
            "agent_ndjson": {
                "command": "caesium-cli agent",
                "framing": "newline-delimited JSON",
                "operations": [
                    "capabilities",
                    "add",
                    "list",
                    "remove",
                    "clear",
                    "get_options",
                    "set_options",
                    "apply_preset",
                    "list_presets",
                    "inspect",
                    "preview",
                    "dry_run",
                    "compress",
                    "retry_failed",
                    "status",
                    "quit"
                ]
            },
            "mcp": {
                "command": "caesium-cli mcp",
                "transport": "stdio (JSON-RPC 2.0, supports NDJSON and Content-Length framing)",
                "protocol_version": "2024-11-05",
                "tools": [
                    "caesium_inspect",
                    "caesium_preview",
                    "caesium_compress",
                    "caesium_convert",
                    "caesium_batch",
                    "caesium_presets",
                    "caesium_retry"
                ],
                "resources": [
                    "caesium://capabilities",
                    "caesium://presets",
                    "caesium://schema/options",
                    "caesium://schema/batch"
                ],
                "prompts": [
                    "optimize-for-web",
                    "fit-under-size-budget",
                    "batch-convert-assets"
                ]
            }
        },
        "exit_codes": {
            "0": { "name": "SUCCESS", "code": exit_codes::SUCCESS },
            "1": { "name": "PARTIAL_FAILURE", "code": exit_codes::PARTIAL_FAILURE },
            "2": { "name": "INVALID_ARGS", "code": exit_codes::INVALID_ARGS },
            "3": { "name": "NO_INPUTS", "code": exit_codes::NO_INPUTS },
            "4": { "name": "OVERWRITE_REFUSED", "code": exit_codes::OVERWRITE_REFUSED },
            "5": { "name": "RUNTIME_ERROR", "code": exit_codes::RUNTIME_ERROR }
        }
    })
}

/// Generates Draft 2020-12 JSON Schema for a named schema target.
pub fn schema_for_target(target: &str) -> CliResult<Value> {
    let val = match target.to_ascii_lowercase().as_str() {
        "options" | "compression-options" => serde_json::to_value(schema_for!(CompressionOptions))?,
        "batch" | "job" | "job-spec" => serde_json::to_value(schema_for!(BatchJobSpec))?,
        "report" | "batch-report" => serde_json::to_value(schema_for!(BatchReport))?,
        "inspect" | "image-info" => serde_json::to_value(schema_for!(ImageInfo))?,
        "preview" => serde_json::to_value(schema_for!(PreviewResult))?,
        "dry-run" | "dry_run" => serde_json::to_value(schema_for!(DryRunPlan))?,
        "receipt" => serde_json::to_value(schema_for!(JobReceipt))?,
        "all" => json!({
            "schema_version": SCHEMA_VERSION,
            "options": schema_for!(CompressionOptions),
            "batch": schema_for!(BatchJobSpec),
            "report": schema_for!(BatchReport),
            "inspect": schema_for!(ImageInfo),
            "preview": schema_for!(PreviewResult),
            "dry_run": schema_for!(DryRunPlan),
            "receipt": schema_for!(JobReceipt),
        }),
        other => {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!("Unknown schema target '{other}'"),
            )
            .with_hint(
                "Valid targets: options, batch, report, inspect, preview, dry-run, receipt, all",
            ));
        }
    };
    Ok(val)
}

pub fn format_human_report(report: &BatchReport) -> String {
    let mut lines = Vec::new();
    for w in &report.warnings {
        lines.push(format!("warning: {w}"));
    }
    for item in &report.items {
        lines.push(format_human_item(item));
    }
    let s = &report.summary;
    lines.push(String::new());
    lines.push(format!(
        "Summary: {} total ({} compressed, {} skipped, {} warnings, {} failed{}) | {} -> {} | saved {} ({:.2}%) in {} ms",
        s.total,
        s.succeeded,
        s.skipped,
        s.warnings,
        s.failed,
        if s.canceled > 0 {
            format!(", {} canceled", s.canceled)
        } else {
            String::new()
        },
        format_human_size(s.total_original_bytes as i64),
        format_human_size(s.total_compressed_bytes as i64),
        format_human_size(s.saved_bytes),
        s.saved_percent,
        s.elapsed_ms
    ));
    lines.join("\n")
}

pub fn format_human_item(item: &ItemResult) -> String {
    let tag = match item.status {
        ItemStatus::Compressed => "OK  ",
        ItemStatus::Skipped => "SKIP",
        ItemStatus::Warning => "WARN",
        ItemStatus::Error => "ERR ",
        ItemStatus::Canceled => "STOP",
    };
    let name = item
        .input_path
        .file_name()
        .map(|s| s.to_string_lossy().to_string())
        .unwrap_or_else(|| item.input_path.to_string_lossy().to_string());
    let detail = match &item.message {
        Some(m) => format!(" — {m}"),
        None => String::new(),
    };
    format!(
        "[{tag}] {name} ({} -> {}, {:.1}%, {}x{} -> {}x{}){detail}",
        format_human_size(item.original_size as i64),
        format_human_size(item.compressed_size as i64),
        -item.saved_percent,
        item.original_dimensions.0,
        item.original_dimensions.1,
        item.compressed_dimensions.0,
        item.compressed_dimensions.1
    )
}

pub fn format_human_dry_run(plan: &DryRunPlan) -> String {
    let mut lines = Vec::new();
    for w in &plan.warnings {
        lines.push(format!("warning: {w}"));
    }
    lines.push(format!(
        "Dry run plan ({} files, root: {}):",
        plan.items.len(),
        plan.root_folder.display()
    ));
    for item in &plan.items {
        let flags = if item.would_overwrite_input {
            " [OVERWRITE-INPUT]"
        } else if item.output_exists {
            " [OVERWRITE-EXISTING]"
        } else {
            ""
        };
        let resize_str = match item.target_resize {
            Some((w, h)) => format!(" resize={w}x{h}"),
            None => String::new(),
        };
        lines.push(format!(
            "  {} -> {} ({} -> {}, {}{}){flags}",
            item.input_path.display(),
            item.output_path.display(),
            item.input_format,
            item.output_format,
            format_human_size(item.original_size as i64),
            resize_str
        ));
    }
    if plan.requires_overwrite {
        lines.push("Note: in-place overwrite detected; pass --overwrite to execute.".to_string());
    }
    lines.join("\n")
}

pub fn format_human_inspect(items: &[ImageInfo]) -> String {
    items
        .iter()
        .map(|info| {
            let meta = match (info.has_exif, info.has_icc_profile) {
                (true, true) => "EXIF+ICC",
                (true, false) => "EXIF",
                (false, true) => "ICC",
                (false, false) => "none",
            };
            format!(
                "{} | {} ({}) | {}x{} (eff {}x{}, {:.2} MP, orient={}) | {} | meta={}",
                info.path,
                info.format.to_uppercase(),
                info.mime,
                info.width,
                info.height,
                info.effective_width,
                info.effective_height,
                info.megapixels,
                info.exif_orientation,
                info.human_size,
                meta
            )
        })
        .collect::<Vec<_>>()
        .join("\n")
}

pub fn format_human_previews(items: &[PreviewResult]) -> String {
    items
        .iter()
        .map(|p| {
            let skip = if p.would_skip_if_bigger {
                " [would skip: bigger]"
            } else {
                ""
            };
            format!(
                "{} | {} -> {} | {} -> {} (saved {}, {:.2}%) | {}x{} -> {}x{}{skip}",
                p.input_path.display(),
                p.input_format,
                p.output_format,
                format_human_size(p.original_size as i64),
                format_human_size(p.compressed_size as i64),
                format_human_size(p.saved_bytes),
                p.saved_percent,
                p.original_dimensions.0,
                p.original_dimensions.1,
                p.compressed_dimensions.0,
                p.compressed_dimensions.1
            )
        })
        .collect::<Vec<_>>()
        .join("\n")
}

pub fn format_human_presets(presets: &[PresetEntry]) -> String {
    presets
        .iter()
        .map(|p| {
            let kind = if p.builtin { "builtin" } else { "custom" };
            format!("{:<14} [{:<7}] {}", p.name, kind, p.description)
        })
        .collect::<Vec<_>>()
        .join("\n")
}
