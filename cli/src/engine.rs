use crate::error::{CaesiumCliError, CliResult, ErrorCode};
use crate::inspect::{is_rotated_by_exif, read_exif_orientation};
use crate::options::{
    CompressionMode, CompressionOptions, KeepDatesOptions, MoveOriginalAction, ResizeMode,
};
use crate::scanner::common_root_folder;
use crate::SCHEMA_VERSION;
use caesium::SupportedFileTypes;
use chrono::Utc;
use image::{ColorType, DynamicImage, ImageFormat, ImageReader};
use rayon::prelude::*;
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};
use std::fs::{self, File, FileTimes, OpenOptions};
use std::io::Cursor;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::{Instant, SystemTime};
use tempfile::Builder as TempBuilder;

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, JsonSchema)]
#[serde(tag = "kind", content = "path", rename_all = "snake_case")]
pub enum OutputTarget {
    Directory(PathBuf),
    SingleFile(PathBuf),
    SameFolder,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, JsonSchema)]
#[serde(rename_all = "snake_case")]
pub enum ItemStatus {
    Compressed,
    Skipped,
    Warning,
    Error,
    Canceled,
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct ItemResult {
    pub input_path: PathBuf,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub output_path: Option<PathBuf>,
    pub input_format: String,
    pub output_format: String,
    pub original_size: u64,
    pub compressed_size: u64,
    pub saved_bytes: i64,
    pub saved_percent: f64,
    pub original_dimensions: (u32, u32),
    pub compressed_dimensions: (u32, u32),
    pub status: ItemStatus,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub message: Option<String>,
    pub elapsed_ms: u64,
}

#[derive(Debug, Clone, Default, Serialize, Deserialize, JsonSchema)]
pub struct BatchSummary {
    pub total: usize,
    pub succeeded: usize,
    pub skipped: usize,
    pub warnings: usize,
    pub failed: usize,
    pub canceled: usize,
    pub total_original_bytes: u64,
    pub total_compressed_bytes: u64,
    pub saved_bytes: i64,
    pub saved_percent: f64,
    pub elapsed_ms: u64,
}

impl BatchSummary {
    pub fn from_items(items: &[ItemResult], elapsed_ms: u64) -> Self {
        let mut s = Self {
            total: items.len(),
            elapsed_ms,
            ..Default::default()
        };
        for item in items {
            match item.status {
                ItemStatus::Compressed => s.succeeded += 1,
                ItemStatus::Skipped => s.skipped += 1,
                ItemStatus::Warning => s.warnings += 1,
                ItemStatus::Error => s.failed += 1,
                ItemStatus::Canceled => s.canceled += 1,
            }
            s.total_original_bytes = s.total_original_bytes.saturating_add(item.original_size);
            let eff_compressed = if matches!(item.status, ItemStatus::Error | ItemStatus::Canceled)
            {
                item.original_size
            } else {
                item.compressed_size
            };
            s.total_compressed_bytes = s.total_compressed_bytes.saturating_add(eff_compressed);
        }
        s.saved_bytes = (s.total_original_bytes as i64) - (s.total_compressed_bytes as i64);
        s.saved_percent = if s.total_original_bytes > 0 {
            ((s.saved_bytes as f64) / (s.total_original_bytes as f64) * 10000.0).round() / 100.0
        } else {
            0.0
        };
        s
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct BatchReport {
    pub summary: BatchSummary,
    pub items: Vec<ItemResult>,
    pub warnings: Vec<String>,
    pub root_folder: PathBuf,
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct DryRunItem {
    pub input_path: PathBuf,
    pub output_path: PathBuf,
    pub input_format: String,
    pub output_format: String,
    pub original_size: u64,
    pub original_dimensions: (u32, u32),
    pub target_resize: Option<(u32, u32)>,
    pub would_overwrite_input: bool,
    pub output_exists: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct DryRunPlan {
    pub items: Vec<DryRunItem>,
    pub root_folder: PathBuf,
    pub warnings: Vec<String>,
    pub requires_overwrite: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct PreviewResult {
    pub input_path: PathBuf,
    pub input_format: String,
    pub output_format: String,
    pub original_size: u64,
    pub compressed_size: u64,
    pub saved_bytes: i64,
    pub saved_percent: f64,
    pub original_dimensions: (u32, u32),
    pub compressed_dimensions: (u32, u32),
    pub would_skip_if_bigger: bool,
    pub elapsed_ms: u64,
    pub warnings: Vec<String>,
}

#[derive(Debug, Clone)]
pub struct MemoryResult {
    pub bytes: Vec<u8>,
    pub input_format: String,
    pub output_format: String,
    pub original_size: u64,
    pub compressed_size: u64,
    pub saved_bytes: i64,
    pub saved_percent: f64,
    pub warnings: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct JobReceipt {
    pub schema_version: String,
    pub created_at: String,
    pub output_target: OutputTarget,
    pub root_folder: PathBuf,
    pub options: CompressionOptions,
    pub summary: BatchSummary,
    pub items: Vec<ItemResult>,
    pub failed_inputs: Vec<PathBuf>,
}

impl JobReceipt {
    pub fn from_report(
        report: &BatchReport,
        output_target: OutputTarget,
        options: &CompressionOptions,
    ) -> Self {
        let failed_inputs = report
            .items
            .iter()
            .filter(|i| matches!(i.status, ItemStatus::Error | ItemStatus::Canceled))
            .map(|i| i.input_path.clone())
            .collect();
        Self {
            schema_version: SCHEMA_VERSION.to_string(),
            created_at: Utc::now().to_rfc3339(),
            output_target,
            root_folder: report.root_folder.clone(),
            options: options.clone(),
            summary: report.summary.clone(),
            items: report.items.clone(),
            failed_inputs,
        }
    }

    pub fn save(&self, path: &Path) -> CliResult<()> {
        if let Some(parent) = path.parent() {
            if !parent.as_os_str().is_empty() {
                fs::create_dir_all(parent)?;
            }
        }
        fs::write(path, serde_json::to_string_pretty(self)?)?;
        Ok(())
    }

    pub fn load(path: &Path) -> CliResult<Self> {
        let raw = fs::read_to_string(path).map_err(|e| {
            CaesiumCliError::new(
                ErrorCode::InvalidInput,
                format!("Cannot read job receipt {}: {e}", path.display()),
            )
        })?;
        serde_json::from_str(&raw).map_err(|e| {
            CaesiumCliError::new(
                ErrorCode::InvalidInput,
                format!("Invalid job receipt {}: {e}", path.display()),
            )
        })
    }
}

pub fn detect_filetype(bytes: &[u8]) -> SupportedFileTypes {
    match infer::get(bytes) {
        Some(kind) => match kind.mime_type() {
            "image/jpeg" => SupportedFileTypes::Jpeg,
            "image/png" => SupportedFileTypes::Png,
            "image/webp" => SupportedFileTypes::WebP,
            "image/tiff" => SupportedFileTypes::Tiff,
            "image/gif" => SupportedFileTypes::Gif,
            _ => SupportedFileTypes::Unkn,
        },
        None => SupportedFileTypes::Unkn,
    }
}

pub fn filetype_name(ft: SupportedFileTypes) -> &'static str {
    match ft {
        SupportedFileTypes::Jpeg => "jpg",
        SupportedFileTypes::Png => "png",
        SupportedFileTypes::WebP => "webp",
        SupportedFileTypes::Tiff => "tiff",
        SupportedFileTypes::Gif => "gif",
        SupportedFileTypes::Unkn => "unknown",
    }
}

/// Computes target `(width, height)` passed to `libcaesium` (`0` means auto-aspect).
/// Mirrors `Utils::cResize` from the Caesium desktop GUI, including EXIF orientation.
pub fn compute_resize_target(
    raw_width: u32,
    raw_height: u32,
    exif_orientation: u32,
    options: &CompressionOptions,
) -> (u32, u32) {
    if !options.resize.is_active() || raw_width == 0 || raw_height == 0 {
        return (0, 0);
    }

    let r = &options.resize;
    let rotated = is_rotated_by_exif(exif_orientation);
    let (eff_w, eff_h) = if rotated {
        (raw_height, raw_width)
    } else {
        (raw_width, raw_height)
    };

    let (target_w, target_h) = match r.mode {
        ResizeMode::None => (0, 0),
        ResizeMode::Dimensions => {
            if r.do_not_enlarge && (r.width >= eff_w || r.height >= eff_h) {
                return (0, 0);
            }
            (r.width, r.height)
        }
        ResizeMode::Percentage => {
            if r.do_not_enlarge && (r.width >= 100 || r.height >= 100) {
                return (0, 0);
            }
            let out_w = ((eff_w as f64) * (r.width as f64) / 100.0).round().max(1.0) as u32;
            let out_h = ((eff_h as f64) * (r.height as f64) / 100.0)
                .round()
                .max(1.0) as u32;
            (out_w, out_h)
        }
        ResizeMode::FixedWidth => {
            if r.do_not_enlarge && r.width >= eff_w {
                return (0, 0);
            }
            (r.width, 0)
        }
        ResizeMode::FixedHeight => {
            if r.do_not_enlarge && r.height >= eff_h {
                return (0, 0);
            }
            (0, r.height)
        }
        ResizeMode::LongEdge => {
            if eff_w >= eff_h {
                if r.do_not_enlarge && eff_w <= r.edge {
                    return (0, 0);
                }
                (r.edge, 0)
            } else {
                if r.do_not_enlarge && eff_h <= r.edge {
                    return (0, 0);
                }
                (0, r.edge)
            }
        }
        ResizeMode::ShortEdge => {
            if eff_w <= eff_h {
                if r.do_not_enlarge && eff_w <= r.edge {
                    return (0, 0);
                }
                (r.edge, 0)
            } else {
                if r.do_not_enlarge && eff_h <= r.edge {
                    return (0, 0);
                }
                (0, r.edge)
            }
        }
    };

    if target_w == eff_w && target_h == eff_h {
        (0, 0)
    } else {
        (target_w, target_h)
    }
}

/// Resolves the output path for an input file given `OutputTarget`, `root_folder`, and options.
pub fn resolve_output_path(
    input: &Path,
    target: &OutputTarget,
    root_folder: &Path,
    options: &CompressionOptions,
    fallback_original_ext: bool,
) -> CliResult<PathBuf> {
    if let OutputTarget::SingleFile(path) = target {
        return Ok(path.clone());
    }

    let orig_ext = input
        .extension()
        .and_then(|s| s.to_str())
        .unwrap_or("jpg")
        .to_string();
    let out_ext = if fallback_original_ext {
        orig_ext
    } else {
        options
            .format
            .extension()
            .map(|s| s.to_string())
            .unwrap_or(orig_ext)
    };

    let stem = input
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("output");
    let filename = format!("{stem}{}.{out_ext}", options.suffix);

    let out_dir = match target {
        OutputTarget::SameFolder => input
            .parent()
            .map(Path::to_path_buf)
            .unwrap_or_else(|| PathBuf::from(".")),
        OutputTarget::Directory(base_out) => {
            if options.keep_structure {
                let input_parent = input.parent().unwrap_or_else(|| Path::new(""));
                let rel = input_parent
                    .strip_prefix(root_folder)
                    .unwrap_or(Path::new(""));
                base_out.join(rel)
            } else {
                base_out.clone()
            }
        }
        OutputTarget::SingleFile(_) => unreachable!(),
    };

    Ok(out_dir.join(filename))
}

fn paths_refer_to_same_file(a: &Path, b: &Path) -> bool {
    let ca = a.canonicalize().unwrap_or_else(|_| a.to_path_buf());
    let cb = b.canonicalize().unwrap_or_else(|_| b.to_path_buf());
    ca == cb
}

fn image_dimensions_from_bytes(data: &[u8]) -> (u32, u32) {
    let raw = ImageReader::new(Cursor::new(data))
        .with_guessed_format()
        .ok()
        .and_then(|r| r.into_dimensions().ok())
        .unwrap_or((0, 0));
    let orient = read_exif_orientation(data);
    if is_rotated_by_exif(orient) {
        (raw.1, raw.0)
    } else {
        raw
    }
}

/// Core in-memory compression/conversion pipeline used by stdin/stdout streaming,
/// preview mode, and file-based compression.
pub fn compress_bytes(input_bytes: &[u8], options: &CompressionOptions) -> CliResult<MemoryResult> {
    options.validate()?;
    let input_ft = detect_filetype(input_bytes);
    if input_ft == SupportedFileTypes::Unkn {
        return Err(CaesiumCliError::new(
            ErrorCode::UnsupportedFormat,
            "Unsupported or unrecognized image format",
        ));
    }

    let raw_dims = ImageReader::new(Cursor::new(input_bytes))
        .with_guessed_format()
        .ok()
        .and_then(|r| r.into_dimensions().ok())
        .unwrap_or((0, 0));
    let orientation = read_exif_orientation(input_bytes);
    let (target_w, target_h) = compute_resize_target(raw_dims.0, raw_dims.1, orientation, options);

    let mut cs_params = options.to_cs_parameters(target_w, target_h);

    let target_ft = options.format.to_caesium().unwrap_or(input_ft);
    let converting = target_ft != input_ft;

    let output_bytes: Vec<u8> = match (options.mode, converting, target_ft) {
        // GIF compression in libcaesium uses gifsicle on files (not in-memory)
        (_, false, SupportedFileTypes::Gif) => {
            compress_via_temp_files(input_bytes, "gif", &cs_params, None)?
        }
        (_, true, SupportedFileTypes::Gif) => {
            let converted = transcode_with_image_crate(input_bytes, ImageFormat::Gif)?;
            compress_via_temp_files(&converted, "gif", &cs_params, None)?
        }
        (CompressionMode::Quality, false, _) => {
            caesium::compress_in_memory(input_bytes.to_vec(), &cs_params).map_err(|e| {
                CaesiumCliError::new(
                    ErrorCode::CompressionFailed,
                    format!("libcaesium error {}: {}", e.code, e.message),
                )
            })?
        }
        (CompressionMode::Quality, true, fmt) => {
            caesium::convert_in_memory(input_bytes.to_vec(), &cs_params, fmt).map_err(|e| {
                CaesiumCliError::new(
                    ErrorCode::ConversionFailed,
                    format!("libcaesium convert error {}: {}", e.code, e.message),
                )
            })?
        }
        (CompressionMode::Size, false, _) => {
            let max_bytes = options.max_output_size.to_bytes(input_bytes.len() as u64);
            if !(cs_params.width > 0 || cs_params.height > 0) && input_bytes.len() <= max_bytes {
                input_bytes.to_vec()
            } else {
                caesium::compress_to_size_in_memory(
                    input_bytes.to_vec(),
                    &mut cs_params,
                    max_bytes,
                    options.return_smallest,
                )
                .map_err(|e| {
                    CaesiumCliError::new(
                        ErrorCode::CompressionFailed,
                        format!("libcaesium size-mode error {}: {}", e.code, e.message),
                    )
                })?
            }
        }
        (CompressionMode::Size, true, fmt) => {
            let target_img_fmt = map_image_format(fmt)?;
            let intermediate = transcode_with_image_crate(input_bytes, target_img_fmt)?;
            let max_bytes = options.max_output_size.to_bytes(input_bytes.len() as u64);
            caesium::compress_to_size_in_memory(
                intermediate,
                &mut cs_params,
                max_bytes,
                options.return_smallest,
            )
            .map_err(|e| {
                CaesiumCliError::new(
                    ErrorCode::CompressionFailed,
                    format!("libcaesium convert+size error {}: {}", e.code, e.message),
                )
            })?
        }
    };

    let orig_size = input_bytes.len() as u64;
    let comp_size = output_bytes.len() as u64;
    let saved_bytes = (orig_size as i64) - (comp_size as i64);
    let saved_percent = if orig_size > 0 {
        ((saved_bytes as f64) / (orig_size as f64) * 10000.0).round() / 100.0
    } else {
        0.0
    };

    Ok(MemoryResult {
        bytes: output_bytes,
        input_format: filetype_name(input_ft).to_string(),
        output_format: filetype_name(target_ft).to_string(),
        original_size: orig_size,
        compressed_size: comp_size,
        saved_bytes,
        saved_percent,
        warnings: options.warnings(),
    })
}

fn map_image_format(fmt: SupportedFileTypes) -> CliResult<ImageFormat> {
    match fmt {
        SupportedFileTypes::Jpeg => Ok(ImageFormat::Jpeg),
        SupportedFileTypes::Png => Ok(ImageFormat::Png),
        SupportedFileTypes::WebP => Ok(ImageFormat::WebP),
        SupportedFileTypes::Tiff => Ok(ImageFormat::Tiff),
        SupportedFileTypes::Gif => Ok(ImageFormat::Gif),
        SupportedFileTypes::Unkn => Err(CaesiumCliError::new(
            ErrorCode::UnsupportedFormat,
            "Unknown target format",
        )),
    }
}

fn transcode_with_image_crate(input_bytes: &[u8], target: ImageFormat) -> CliResult<Vec<u8>> {
    let mut img = ImageReader::new(Cursor::new(input_bytes))
        .with_guessed_format()
        .map_err(|e| CaesiumCliError::new(ErrorCode::ConversionFailed, e.to_string()))?
        .decode()
        .map_err(|e| CaesiumCliError::new(ErrorCode::ConversionFailed, e.to_string()))?;

    if target == ImageFormat::Jpeg {
        img = match img.color() {
            ColorType::Rgba8 => DynamicImage::from(img.to_rgb8()),
            ColorType::Rgba16 => DynamicImage::from(img.to_rgb16()),
            ColorType::Rgba32F => DynamicImage::from(img.to_rgb32f()),
            _ => img,
        };
    }

    let mut out = Vec::new();
    img.write_to(&mut Cursor::new(&mut out), target)
        .map_err(|e| CaesiumCliError::new(ErrorCode::ConversionFailed, e.to_string()))?;
    Ok(out)
}

fn compress_via_temp_files(
    input_bytes: &[u8],
    ext: &str,
    params: &caesium::parameters::CSParameters,
    size_limit: Option<(usize, bool)>,
) -> CliResult<Vec<u8>> {
    let tmp_dir = tempfile::tempdir()?;
    let in_path = tmp_dir.path().join(format!("in.{ext}"));
    let out_path = tmp_dir.path().join(format!("out.{ext}"));
    fs::write(&in_path, input_bytes)?;

    let res = if let Some((max_bytes, return_smallest)) = size_limit {
        let mut p = *params;
        caesium::compress_to_size(
            in_path.to_string_lossy().to_string(),
            out_path.to_string_lossy().to_string(),
            &mut p,
            max_bytes,
            return_smallest,
        )
    } else {
        caesium::compress(
            in_path.to_string_lossy().to_string(),
            out_path.to_string_lossy().to_string(),
            params,
        )
    };

    res.map_err(|e| {
        CaesiumCliError::new(
            ErrorCode::CompressionFailed,
            format!("libcaesium error {}: {}", e.code, e.message),
        )
    })?;

    Ok(fs::read(&out_path)?)
}

/// Runs an in-memory compression preview for one file without writing any output files.
pub fn preview_file(input: &Path, options: &CompressionOptions) -> CliResult<PreviewResult> {
    options.validate()?;
    let started = Instant::now();
    let raw = fs::read(input).map_err(|e| {
        CaesiumCliError::new(ErrorCode::IoError, format!("Cannot read input: {e}"))
            .with_path(input.to_string_lossy())
    })?;
    let orig_dims = image_dimensions_from_bytes(&raw);
    let mem = compress_bytes(&raw, options)?;
    let comp_dims = image_dimensions_from_bytes(&mem.bytes);
    let would_skip = options.skip_if_bigger && mem.compressed_size >= mem.original_size;

    Ok(PreviewResult {
        input_path: input.to_path_buf(),
        input_format: mem.input_format,
        output_format: mem.output_format,
        original_size: mem.original_size,
        compressed_size: mem.compressed_size,
        saved_bytes: mem.saved_bytes,
        saved_percent: mem.saved_percent,
        original_dimensions: orig_dims,
        compressed_dimensions: comp_dims,
        would_skip_if_bigger: would_skip,
        elapsed_ms: started.elapsed().as_millis() as u64,
        warnings: mem.warnings,
    })
}

/// Plans every input -> output mapping without mutating any files on disk.
pub fn dry_run_batch(
    inputs: &[PathBuf],
    target: OutputTarget,
    options: &CompressionOptions,
    root_override: Option<PathBuf>,
) -> CliResult<DryRunPlan> {
    options.validate()?;
    if inputs.is_empty() {
        return Err(CaesiumCliError::new(
            ErrorCode::NoMatchingInputs,
            "No matching input images found",
        ));
    }
    if matches!(target, OutputTarget::SingleFile(_)) && inputs.len() > 1 {
        return Err(CaesiumCliError::new(
            ErrorCode::InvalidOption,
            "Single-file output path cannot be used with multiple input files",
        )
        .with_hint("Pass a directory to -o/--output when compressing multiple files"));
    }

    let root = root_override.unwrap_or_else(|| common_root_folder(inputs));
    let mut items = Vec::with_capacity(inputs.len());
    let mut requires_overwrite = false;

    for input in inputs {
        let raw = fs::read(input).unwrap_or_default();
        let ft = detect_filetype(&raw);
        let in_fmt = filetype_name(ft).to_string();
        let out_fmt = options
            .format
            .to_caesium()
            .map(filetype_name)
            .unwrap_or(filetype_name(ft))
            .to_string();
        let orig_dims = image_dimensions_from_bytes(&raw);
        let orient = read_exif_orientation(&raw);
        let raw_dims = ImageReader::new(Cursor::new(&raw))
            .with_guessed_format()
            .ok()
            .and_then(|r| r.into_dimensions().ok())
            .unwrap_or((0, 0));
        let (tw, th) = compute_resize_target(raw_dims.0, raw_dims.1, orient, options);
        let out_path = resolve_output_path(input, &target, &root, options, false)?;
        let would_overwrite_input = paths_refer_to_same_file(input, &out_path);
        if would_overwrite_input && !options.overwrite {
            requires_overwrite = true;
        }
        items.push(DryRunItem {
            input_path: input.clone(),
            output_exists: out_path.exists(),
            output_path: out_path,
            input_format: in_fmt,
            output_format: out_fmt,
            original_size: raw.len() as u64,
            original_dimensions: orig_dims,
            target_resize: if tw > 0 || th > 0 {
                Some((tw, th))
            } else {
                None
            },
            would_overwrite_input,
        });
    }

    Ok(DryRunPlan {
        items,
        root_folder: root,
        warnings: options.warnings(),
        requires_overwrite,
    })
}

/// Runs batch compression across inputs using `threads` worker threads.
pub fn run_batch(
    inputs: &[PathBuf],
    target: OutputTarget,
    options: &CompressionOptions,
    threads: usize,
    root_override: Option<PathBuf>,
) -> CliResult<BatchReport> {
    run_batch_with_progress(
        inputs,
        target,
        options,
        threads,
        root_override,
        None,
        |_| {},
    )
}

/// Runs batch compression with optional cancellation token and live per-item progress callback.
pub fn run_batch_with_progress<F>(
    inputs: &[PathBuf],
    target: OutputTarget,
    options: &CompressionOptions,
    threads: usize,
    root_override: Option<PathBuf>,
    cancel: Option<Arc<AtomicBool>>,
    on_item: F,
) -> CliResult<BatchReport>
where
    F: Fn(&ItemResult) + Send + Sync,
{
    options.validate()?;
    if inputs.is_empty() {
        return Err(CaesiumCliError::new(
            ErrorCode::NoMatchingInputs,
            "No matching input images found",
        ));
    }
    if matches!(target, OutputTarget::SingleFile(_)) && inputs.len() > 1 {
        return Err(CaesiumCliError::new(
            ErrorCode::InvalidOption,
            "Single-file output path cannot be used with multiple input files",
        )
        .with_hint("Pass a directory path to -o/--output when processing multiple files"));
    }

    let root = root_override.unwrap_or_else(|| common_root_folder(inputs));

    // Pre-flight overwrite safety check BEFORE mutating any file on disk (mirrors GUI overwrite dialog).
    if !options.overwrite {
        for input in inputs {
            let planned = resolve_output_path(input, &target, &root, options, false)?;
            if paths_refer_to_same_file(input, &planned) {
                return Err(CaesiumCliError::new(
                    ErrorCode::OverwriteRefused,
                    format!(
                        "Refusing to overwrite original image in-place: {}",
                        input.display()
                    ),
                )
                .with_path(input.to_string_lossy())
                .with_hint(
                    "Pass --overwrite to allow replacing originals, or use --suffix / -o <DIR>",
                ));
            }
        }
    }

    let started = Instant::now();
    let num_threads = if threads == 0 {
        std::thread::available_parallelism()
            .map(|n| n.get())
            .unwrap_or(1)
    } else {
        threads
    };

    let pool = rayon::ThreadPoolBuilder::new()
        .num_threads(num_threads)
        .build()
        .map_err(|e| {
            CaesiumCliError::new(
                ErrorCode::CompressionFailed,
                format!("Failed to initialize thread pool: {e}"),
            )
        })?;

    let items: Vec<ItemResult> = pool.install(|| {
        inputs
            .par_iter()
            .map(|input| {
                if let Some(flag) = &cancel {
                    if flag.load(Ordering::Relaxed) {
                        let res = ItemResult {
                            input_path: input.clone(),
                            output_path: None,
                            input_format: String::new(),
                            output_format: String::new(),
                            original_size: fs::metadata(input).map(|m| m.len()).unwrap_or(0),
                            compressed_size: 0,
                            saved_bytes: 0,
                            saved_percent: 0.0,
                            original_dimensions: (0, 0),
                            compressed_dimensions: (0, 0),
                            status: ItemStatus::Canceled,
                            message: Some("Canceled".to_string()),
                            elapsed_ms: 0,
                        };
                        on_item(&res);
                        return res;
                    }
                }
                let res = compress_single_file(input, &target, &root, options);
                on_item(&res);
                res
            })
            .collect()
    });

    let elapsed_ms = started.elapsed().as_millis() as u64;
    let summary = BatchSummary::from_items(&items, elapsed_ms);
    Ok(BatchReport {
        summary,
        items,
        warnings: options.warnings(),
        root_folder: root,
    })
}

fn compress_single_file(
    input: &Path,
    target: &OutputTarget,
    root_folder: &Path,
    options: &CompressionOptions,
) -> ItemResult {
    let started = Instant::now();
    let input_meta = match fs::metadata(input) {
        Ok(m) => m,
        Err(e) => {
            return error_item(
                input,
                0,
                format!("Input file cannot be read: {e}"),
                started.elapsed().as_millis() as u64,
            );
        }
    };
    let original_size = input_meta.len();
    let orig_mtime = input_meta.modified().ok();
    let orig_atime = input_meta.accessed().ok();

    let raw = match fs::read(input) {
        Ok(b) => b,
        Err(e) => {
            return error_item(
                input,
                original_size,
                format!("Failed to read input: {e}"),
                started.elapsed().as_millis() as u64,
            );
        }
    };
    let orig_dims = image_dimensions_from_bytes(&raw);

    let mem = match compress_bytes(&raw, options) {
        Ok(m) => m,
        Err(e) => {
            return ItemResult {
                input_path: input.to_path_buf(),
                output_path: None,
                input_format: filetype_name(detect_filetype(&raw)).to_string(),
                output_format: options.format.canonical_name().to_string(),
                original_size,
                compressed_size: original_size,
                saved_bytes: 0,
                saved_percent: 0.0,
                original_dimensions: orig_dims,
                compressed_dimensions: orig_dims,
                status: ItemStatus::Error,
                message: Some(e.message),
                elapsed_ms: started.elapsed().as_millis() as u64,
            };
        }
    };

    let output_is_bigger = options.skip_if_bigger && mem.compressed_size >= original_size;
    let converting = mem.input_format != mem.output_format;

    let out_path = match resolve_output_path(
        input,
        target,
        root_folder,
        options,
        output_is_bigger && converting,
    ) {
        Ok(p) => p,
        Err(e) => {
            return error_item(
                input,
                original_size,
                e.message,
                started.elapsed().as_millis() as u64,
            );
        }
    };

    if let Some(parent) = out_path.parent() {
        if !parent.as_os_str().is_empty() {
            if let Err(e) = fs::create_dir_all(parent) {
                return error_item(
                    input,
                    original_size,
                    format!("Cannot create output directory {}: {e}", parent.display()),
                    started.elapsed().as_millis() as u64,
                );
            }
        }
    }

    let same_file = paths_refer_to_same_file(input, &out_path);

    let (final_bytes, status, message, final_format) = if output_is_bigger {
        if same_file {
            // Original is already in place; don't touch it
            return ItemResult {
                input_path: input.to_path_buf(),
                output_path: Some(out_path),
                input_format: mem.input_format.clone(),
                output_format: mem.input_format,
                original_size,
                compressed_size: original_size,
                saved_bytes: 0,
                saved_percent: 0.0,
                original_dimensions: orig_dims,
                compressed_dimensions: orig_dims,
                status: ItemStatus::Skipped,
                message: Some("Skipped: compressed file is bigger than original".to_string()),
                elapsed_ms: started.elapsed().as_millis() as u64,
            };
        }
        (
            raw.as_slice(),
            ItemStatus::Skipped,
            Some("Skipped: compressed file is bigger than original (copied original)".to_string()),
            mem.input_format.clone(),
        )
    } else {
        (
            mem.bytes.as_slice(),
            ItemStatus::Compressed,
            None,
            mem.output_format.clone(),
        )
    };

    // Atomic write via temp file in destination directory
    if let Err(e) = atomic_write(&out_path, final_bytes) {
        return error_item(
            input,
            original_size,
            format!("Cannot write output {}: {e}", out_path.display()),
            started.elapsed().as_millis() as u64,
        );
    }

    // Apply file dates if requested
    if options.keep_dates.any() {
        let _ = apply_file_dates(&out_path, options.keep_dates, orig_mtime, orig_atime);
    }

    // Handle move/delete original file (only when output is distinct and compression succeeded)
    let mut final_status = status;
    let mut final_msg = message;
    if !same_file && status == ItemStatus::Compressed {
        match options.move_original {
            MoveOriginalAction::None => {}
            MoveOriginalAction::Trash => {
                if let Err(e) = trash::delete(input) {
                    final_status = ItemStatus::Warning;
                    final_msg = Some(format!(
                        "Compressed, but failed to move original to trash: {e}"
                    ));
                }
            }
            MoveOriginalAction::Delete => {
                if let Err(e) = fs::remove_file(input) {
                    final_status = ItemStatus::Warning;
                    final_msg = Some(format!(
                        "Compressed, but failed to delete original file: {e}"
                    ));
                }
            }
        }
    }

    let comp_size = final_bytes.len() as u64;
    let comp_dims = image_dimensions_from_bytes(final_bytes);
    let saved_bytes = (original_size as i64) - (comp_size as i64);
    let saved_percent = if original_size > 0 {
        ((saved_bytes as f64) / (original_size as f64) * 10000.0).round() / 100.0
    } else {
        0.0
    };

    ItemResult {
        input_path: input.to_path_buf(),
        output_path: Some(out_path),
        input_format: mem.input_format,
        output_format: final_format,
        original_size,
        compressed_size: comp_size,
        saved_bytes,
        saved_percent,
        original_dimensions: orig_dims,
        compressed_dimensions: comp_dims,
        status: final_status,
        message: final_msg,
        elapsed_ms: started.elapsed().as_millis() as u64,
    }
}

fn error_item(input: &Path, original_size: u64, message: String, elapsed_ms: u64) -> ItemResult {
    ItemResult {
        input_path: input.to_path_buf(),
        output_path: None,
        input_format: String::new(),
        output_format: String::new(),
        original_size,
        compressed_size: original_size,
        saved_bytes: 0,
        saved_percent: 0.0,
        original_dimensions: (0, 0),
        compressed_dimensions: (0, 0),
        status: ItemStatus::Error,
        message: Some(message),
        elapsed_ms,
    }
}

fn atomic_write(dest: &Path, bytes: &[u8]) -> std::io::Result<()> {
    let parent = dest
        .parent()
        .filter(|p| !p.as_os_str().is_empty())
        .unwrap_or_else(|| Path::new("."));
    let mut tmp = TempBuilder::new()
        .prefix(".caesium-tmp-")
        .tempfile_in(parent)?;
    use std::io::Write;
    tmp.write_all(bytes)?;
    tmp.flush()?;
    tmp.persist(dest).map_err(|e| e.error)?;
    Ok(())
}

fn apply_file_dates(
    dest: &Path,
    keep: KeepDatesOptions,
    mtime: Option<SystemTime>,
    atime: Option<SystemTime>,
) -> std::io::Result<()> {
    let file: File = OpenOptions::new().write(true).open(dest)?;
    let mut times = FileTimes::new();
    let mut any = false;
    if keep.modified {
        if let Some(m) = mtime {
            times = times.set_modified(m);
            any = true;
        }
    }
    if keep.accessed {
        if let Some(a) = atime {
            times = times.set_accessed(a);
            any = true;
        }
    }
    if any {
        file.set_times(times)?;
    }
    Ok(())
}

/// Retries only the failed/canceled inputs captured in a previous `JobReceipt`.
pub fn retry_from_receipt(receipt_path: &Path, threads: usize) -> CliResult<BatchReport> {
    let receipt = JobReceipt::load(receipt_path)?;
    if receipt.failed_inputs.is_empty() {
        return Ok(BatchReport {
            summary: BatchSummary::default(),
            items: Vec::new(),
            warnings: vec!["No failed items to retry in receipt.".to_string()],
            root_folder: receipt.root_folder,
        });
    }
    run_batch(
        &receipt.failed_inputs,
        receipt.output_target,
        &receipt.options,
        threads,
        Some(receipt.root_folder),
    )
}
