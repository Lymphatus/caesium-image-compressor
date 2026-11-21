use crate::error::{CaesiumCliError, CliResult, ErrorCode};
use crate::options::{ImportFilters, SizeCondition, DEFAULT_MAX_INPUT_BYTES};
use regex::Regex;
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};
use std::collections::HashSet;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::{Component, Path, PathBuf};
use walkdir::WalkDir;

pub const SUPPORTED_EXTENSIONS: [&str; 7] = ["jpg", "jpeg", "png", "webp", "tif", "tiff", "gif"];

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct ScanOptions {
    pub recursive: bool,
    pub filters: ImportFilters,
    pub max_input_bytes: Option<u64>,
}

impl Default for ScanOptions {
    fn default() -> Self {
        Self {
            recursive: true,
            filters: ImportFilters::default(),
            max_input_bytes: Some(DEFAULT_MAX_INPUT_BYTES),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct SkippedInput {
    pub path: PathBuf,
    pub reason: String,
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct ScanResult {
    pub files: Vec<PathBuf>,
    pub root_folder: PathBuf,
    pub skipped: Vec<SkippedInput>,
}

/// Computes the deepest common ancestor directory for a list of file or folder paths.
/// Mirrors `Importer::getRootFolder` in the Qt desktop app.
pub fn common_root_folder(paths: &[PathBuf]) -> PathBuf {
    if paths.is_empty() {
        return PathBuf::from("/");
    }

    // For files (paths that exist and are files, or look like files with extensions when virtual),
    // use their parent directory if appropriate; for directories keep as-is.
    let dir_paths: Vec<PathBuf> = paths
        .iter()
        .map(|p| {
            if p.is_file() {
                p.parent()
                    .map(Path::to_path_buf)
                    .unwrap_or_else(|| p.clone())
            } else if !p.exists() && p.extension().is_some() {
                p.parent()
                    .map(Path::to_path_buf)
                    .unwrap_or_else(|| p.clone())
            } else {
                p.clone()
            }
        })
        .collect();

    let mut common: Vec<Component<'_>> = dir_paths[0].components().collect();
    for path in &dir_paths[1..] {
        let comps: Vec<Component<'_>> = path.components().collect();
        let mut matched = 0;
        for (a, b) in common.iter().zip(comps.iter()) {
            if a == b {
                matched += 1;
            } else {
                break;
            }
        }
        common.truncate(matched);
    }

    if common.is_empty() {
        PathBuf::from("/")
    } else {
        common.iter().collect()
    }
}

/// Checks whether a candidate file passes GUI-equivalent Advanced Import filters.
pub fn passes_filters(path: &Path, size_bytes: u64, filters: &ImportFilters) -> CliResult<bool> {
    if let Some(size_filter) = filters.skip_by_size {
        let should_skip = match size_filter.condition {
            SizeCondition::GreaterThan => size_bytes > size_filter.bytes,
            SizeCondition::EqualTo => size_bytes == size_filter.bytes,
            SizeCondition::LessThan => size_bytes < size_filter.bytes,
        };
        if should_skip {
            return Ok(false);
        }
    }

    if let Some(pattern) = &filters.filename_regex {
        if !pattern.is_empty() {
            let regex = Regex::new(pattern).map_err(|e| {
                CaesiumCliError::new(
                    ErrorCode::InvalidOption,
                    format!("Invalid filename regex '{pattern}': {e}"),
                )
            })?;
            let fname = path
                .file_name()
                .map(|n| n.to_string_lossy())
                .unwrap_or_default();
            if !regex.is_match(&fname) {
                return Ok(false);
            }
        }
    }

    Ok(true)
}

pub fn has_supported_extension(path: &Path) -> bool {
    path.extension()
        .and_then(|e| e.to_str())
        .map(|ext| {
            let lower = ext.to_ascii_lowercase();
            SUPPORTED_EXTENSIONS.contains(&lower.as_str())
        })
        .unwrap_or(false)
}

pub fn is_supported_magic(path: &Path) -> bool {
    match infer::get_from_path(path) {
        Ok(Some(kind)) => matches!(
            kind.mime_type(),
            "image/jpeg" | "image/png" | "image/webp" | "image/tiff" | "image/gif"
        ),
        _ => false,
    }
}

/// Scans input files and directories, applying recursive traversal, format checks,
/// size limits, import filters, and canonical-path deduplication.
pub fn scan_inputs(inputs: &[PathBuf], options: &ScanOptions) -> CliResult<ScanResult> {
    // Validate regex upfront before walking the filesystem
    if let Some(pattern) = &options.filters.filename_regex {
        if !pattern.is_empty() {
            Regex::new(pattern).map_err(|e| {
                CaesiumCliError::new(
                    ErrorCode::InvalidOption,
                    format!("Invalid filename regex '{pattern}': {e}"),
                )
            })?;
        }
    }

    let mut files = Vec::new();
    let mut skipped = Vec::new();
    let mut seen: HashSet<PathBuf> = HashSet::new();

    for input in inputs {
        if !input.exists() {
            skipped.push(SkippedInput {
                path: input.clone(),
                reason: "Path does not exist".to_string(),
            });
            continue;
        }

        if input.is_file() {
            consider_file(input, true, options, &mut seen, &mut files, &mut skipped)?;
        } else if input.is_dir() {
            let mut walker = WalkDir::new(input);
            if !options.recursive {
                walker = walker.max_depth(1);
            }
            for entry in walker.into_iter().filter_map(Result::ok) {
                if entry.file_type().is_file() {
                    consider_file(
                        entry.path(),
                        false,
                        options,
                        &mut seen,
                        &mut files,
                        &mut skipped,
                    )?;
                }
            }
        }
    }

    files.sort();
    let root_folder = if files.is_empty() {
        common_root_folder(inputs)
    } else {
        common_root_folder(&files)
    };

    Ok(ScanResult {
        files,
        root_folder,
        skipped,
    })
}

fn consider_file(
    path: &Path,
    explicit_file: bool,
    options: &ScanOptions,
    seen: &mut HashSet<PathBuf>,
    files: &mut Vec<PathBuf>,
    skipped: &mut Vec<SkippedInput>,
) -> CliResult<()> {
    // For directory scans, require supported extension first to avoid opening arbitrary binary/text files
    if !explicit_file && !has_supported_extension(path) {
        return Ok(());
    }

    let canonical = path.canonicalize().unwrap_or_else(|_| path.to_path_buf());
    if !seen.insert(canonical.clone()) {
        return Ok(());
    }

    let meta = match std::fs::metadata(&canonical) {
        Ok(m) => m,
        Err(e) => {
            skipped.push(SkippedInput {
                path: canonical,
                reason: format!("Cannot read metadata: {e}"),
            });
            return Ok(());
        }
    };

    if explicit_file && !has_supported_extension(&canonical) && !is_supported_magic(&canonical) {
        skipped.push(SkippedInput {
            path: canonical,
            reason: "Unsupported file format".to_string(),
        });
        return Ok(());
    }

    if let Some(max_bytes) = options.max_input_bytes {
        if max_bytes > 0 && meta.len() > max_bytes {
            skipped.push(SkippedInput {
                path: canonical,
                reason: format!("File exceeds max input size limit ({max_bytes} bytes)"),
            });
            return Ok(());
        }
    }

    if !passes_filters(&canonical, meta.len(), &options.filters)? {
        skipped.push(SkippedInput {
            path: canonical,
            reason: "Excluded by import filter".to_string(),
        });
        return Ok(());
    }

    files.push(canonical);
    Ok(())
}

/// Loads newline-separated file/folder paths from a list file, skipping comments,
/// empty lines, non-existent entries, and duplicates (matching `AdvancedImportDialog::openList`).
pub fn load_paths_from_list(list_path: &Path) -> CliResult<Vec<PathBuf>> {
    let file = File::open(list_path).map_err(|e| {
        CaesiumCliError::new(
            ErrorCode::IoError,
            format!("Cannot open list file {}: {e}", list_path.display()),
        )
    })?;
    load_paths_from_reader(BufReader::new(file))
}

pub fn load_paths_from_reader<R: BufRead>(reader: R) -> CliResult<Vec<PathBuf>> {
    let mut seen = HashSet::new();
    let mut out = Vec::new();
    for line in reader.lines() {
        let raw = line?;
        let trimmed = raw.trim();
        if trimmed.is_empty() || trimmed.starts_with('#') {
            continue;
        }
        let path = PathBuf::from(trimmed);
        if !path.exists() {
            continue;
        }
        let canonical = path.canonicalize().unwrap_or(path);
        if seen.insert(canonical.clone()) {
            out.push(canonical);
        }
    }
    Ok(out)
}
