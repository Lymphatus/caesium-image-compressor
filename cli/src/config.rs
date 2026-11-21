use crate::error::{CaesiumCliError, CliResult, ErrorCode};
use crate::options::{
    ChromaSubsamplingOption, CompressionMode, CompressionOptions, KeepDatesOptions, MaxOutputSize,
    MaxOutputSizeUnit, MoveOriginalAction, OutputFormat, ResizeMode, ResizeOptions,
    TiffCompressionOption, TiffDeflateLevelOption,
};
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct PresetEntry {
    pub name: String,
    pub description: String,
    pub builtin: bool,
    pub options: CompressionOptions,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
struct PresetFile {
    presets: BTreeMap<String, StoredPreset>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct StoredPreset {
    description: String,
    options: CompressionOptions,
}

pub struct PresetStore {
    dir: PathBuf,
}

impl PresetStore {
    pub fn default_store() -> Self {
        let dir = dirs::config_dir()
            .unwrap_or_else(|| PathBuf::from("."))
            .join("caesium");
        Self { dir }
    }

    pub fn in_dir(dir: &Path) -> Self {
        Self {
            dir: dir.to_path_buf(),
        }
    }

    fn file_path(&self) -> PathBuf {
        self.dir.join("presets.json")
    }

    pub fn builtins() -> Vec<PresetEntry> {
        let gui_default = CompressionOptions::default();

        let web = CompressionOptions {
            format: OutputFormat::WebP,
            webp_quality: 75,
            jpeg_quality: 78,
            png_quality: 80,
            keep_metadata: false,
            jpeg_progressive: true,
            jpeg_chroma_subsampling: ChromaSubsamplingOption::Cs420,
            ..Default::default()
        };

        let lossless = CompressionOptions {
            lossless: true,
            keep_metadata: true,
            png_optimization_level: 4,
            tiff_method: TiffCompressionOption::Deflate,
            tiff_deflate_level: TiffDeflateLevelOption::Best,
            ..Default::default()
        };

        let thumbnail = CompressionOptions {
            format: OutputFormat::WebP,
            webp_quality: 72,
            jpeg_quality: 75,
            keep_metadata: false,
            resize: ResizeOptions {
                mode: ResizeMode::LongEdge,
                edge: 400,
                do_not_enlarge: true,
                keep_aspect_ratio: true,
                ..Default::default()
            },
            ..Default::default()
        };

        let social = CompressionOptions {
            format: OutputFormat::Jpg,
            jpeg_quality: 82,
            jpeg_progressive: true,
            keep_metadata: false,
            resize: ResizeOptions {
                mode: ResizeMode::LongEdge,
                edge: 2048,
                do_not_enlarge: true,
                keep_aspect_ratio: true,
                ..Default::default()
            },
            ..Default::default()
        };

        let email = CompressionOptions {
            mode: CompressionMode::Size,
            max_output_size: MaxOutputSize {
                value: 300,
                unit: MaxOutputSizeUnit::Kilobytes,
            },
            keep_metadata: false,
            resize: ResizeOptions {
                mode: ResizeMode::LongEdge,
                edge: 1600,
                do_not_enlarge: true,
                keep_aspect_ratio: true,
                ..Default::default()
            },
            ..Default::default()
        };

        let archive = CompressionOptions {
            lossless: true,
            keep_metadata: true,
            png_optimization_level: 6,
            keep_dates: KeepDatesOptions::all(),
            ..Default::default()
        };

        let aggressive = CompressionOptions {
            format: OutputFormat::WebP,
            webp_quality: 50,
            jpeg_quality: 55,
            png_quality: 55,
            gif_quality: 55,
            keep_metadata: false,
            jpeg_chroma_subsampling: ChromaSubsamplingOption::Cs420,
            ..Default::default()
        };

        vec![
            PresetEntry {
                name: "gui-default".into(),
                description: "Exact defaults from Caesium Image Compressor desktop GUI".into(),
                builtin: true,
                options: gui_default,
            },
            PresetEntry {
                name: "web".into(),
                description:
                    "Web-optimized WebP (Q75), stripped metadata, progressive JPEG fallback".into(),
                builtin: true,
                options: web,
            },
            PresetEntry {
                name: "lossless".into(),
                description: "Lossless optimization keeping metadata and original format".into(),
                builtin: true,
                options: lossless,
            },
            PresetEntry {
                name: "thumbnail".into(),
                description: "400px long-edge WebP thumbnails (Q72, no upscaling)".into(),
                builtin: true,
                options: thumbnail,
            },
            PresetEntry {
                name: "social".into(),
                description: "2048px long-edge progressive JPEG (Q82) ready for social sharing"
                    .into(),
                builtin: true,
                options: social,
            },
            PresetEntry {
                name: "email".into(),
                description: "Target max 300 KB and <=1600px long edge for email attachments"
                    .into(),
                builtin: true,
                options: email,
            },
            PresetEntry {
                name: "archive".into(),
                description:
                    "Maximum lossless PNG/TIFF compression preserving metadata and file dates"
                        .into(),
                builtin: true,
                options: archive,
            },
            PresetEntry {
                name: "aggressive".into(),
                description: "Maximum byte reduction (WebP Q50, stripped metadata)".into(),
                builtin: true,
                options: aggressive,
            },
        ]
    }

    fn load_custom(&self) -> CliResult<PresetFile> {
        let path = self.file_path();
        if !path.exists() {
            return Ok(PresetFile::default());
        }
        let content = fs::read_to_string(&path)?;
        serde_json::from_str(&content).map_err(|e| {
            CaesiumCliError::new(
                ErrorCode::ConfigError,
                format!("Failed to parse presets file {}: {e}", path.display()),
            )
        })
    }

    fn save_custom(&self, file: &PresetFile) -> CliResult<()> {
        fs::create_dir_all(&self.dir)?;
        let content = serde_json::to_string_pretty(file)?;
        fs::write(self.file_path(), content)?;
        Ok(())
    }

    pub fn list(&self) -> CliResult<Vec<PresetEntry>> {
        let mut all = Self::builtins();
        let custom = self.load_custom()?;
        for (name, stored) in custom.presets {
            // Custom preset with same name overrides builtin display? Keep separate if distinct.
            all.retain(|p| p.name != name);
            all.push(PresetEntry {
                name,
                description: stored.description,
                builtin: false,
                options: stored.options,
            });
        }
        Ok(all)
    }

    pub fn get(&self, name: &str) -> CliResult<PresetEntry> {
        let list = self.list()?;
        list.into_iter().find(|p| p.name == name).ok_or_else(|| {
            CaesiumCliError::new(
                ErrorCode::PresetNotFound,
                format!("Preset '{name}' not found"),
            )
            .with_hint("Run `caesium-cli preset list` to view available presets")
        })
    }

    pub fn save(
        &self,
        name: &str,
        description: Option<String>,
        options: &CompressionOptions,
    ) -> CliResult<PresetEntry> {
        let clean = name.trim();
        if clean.is_empty() {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                "Preset name cannot be empty",
            ));
        }
        options.validate()?;
        let mut custom = self.load_custom()?;
        let desc = description.unwrap_or_else(|| format!("Custom preset '{clean}'"));
        custom.presets.insert(
            clean.to_string(),
            StoredPreset {
                description: desc.clone(),
                options: options.clone(),
            },
        );
        self.save_custom(&custom)?;
        Ok(PresetEntry {
            name: clean.to_string(),
            description: desc,
            builtin: false,
            options: options.clone(),
        })
    }

    pub fn delete(&self, name: &str) -> CliResult<()> {
        let mut custom = self.load_custom()?;
        if custom.presets.remove(name).is_some() {
            self.save_custom(&custom)?;
            return Ok(());
        }
        if Self::builtins().iter().any(|p| p.name == name) {
            return Err(CaesiumCliError::new(
                ErrorCode::PresetProtected,
                format!("Cannot delete built-in preset '{name}'"),
            ));
        }
        Err(CaesiumCliError::new(
            ErrorCode::PresetNotFound,
            format!("Preset '{name}' not found"),
        ))
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
#[serde(default)]
pub struct CliConfig {
    pub default_threads: Option<usize>,
    pub recursive_by_default: bool,
    pub max_input_bytes: u64,
    pub defaults: CompressionOptions,
}

impl Default for CliConfig {
    fn default() -> Self {
        Self {
            default_threads: None,
            recursive_by_default: true,
            max_input_bytes: crate::options::DEFAULT_MAX_INPUT_BYTES,
            defaults: CompressionOptions::default(),
        }
    }
}

pub struct ConfigStore {
    dir: PathBuf,
}

impl ConfigStore {
    pub fn default_store() -> Self {
        let dir = dirs::config_dir()
            .unwrap_or_else(|| PathBuf::from("."))
            .join("caesium");
        Self { dir }
    }

    pub fn in_dir(dir: &Path) -> Self {
        Self {
            dir: dir.to_path_buf(),
        }
    }

    pub fn config_path(&self) -> PathBuf {
        self.dir.join("config.json")
    }

    pub fn load(&self) -> CliResult<CliConfig> {
        let path = self.config_path();
        if !path.exists() {
            return Ok(CliConfig::default());
        }
        let text = fs::read_to_string(&path)?;
        serde_json::from_str(&text).map_err(|e| {
            CaesiumCliError::new(
                ErrorCode::ConfigError,
                format!("Failed to parse config {}: {e}", path.display()),
            )
        })
    }

    pub fn save(&self, cfg: &CliConfig) -> CliResult<()> {
        cfg.defaults.validate()?;
        fs::create_dir_all(&self.dir)?;
        fs::write(self.config_path(), serde_json::to_string_pretty(cfg)?)?;
        Ok(())
    }

    pub fn reset(&self) -> CliResult<CliConfig> {
        let cfg = CliConfig::default();
        self.save(&cfg)?;
        Ok(cfg)
    }

    /// Default location of Caesium GUI `QSettings` INI file on Linux.
    pub fn default_gui_ini_path() -> Option<PathBuf> {
        dirs::config_dir().map(|d| d.join("SaeraSoft").join("Caesium Image Compressor.conf"))
    }

    /// Imports compression options directly from Caesium GUI's Qt `QSettings` INI file.
    pub fn import_gui_ini(path: &Path) -> CliResult<CompressionOptions> {
        let raw = fs::read_to_string(path).map_err(|e| {
            CaesiumCliError::new(
                ErrorCode::ConfigError,
                format!("Cannot read GUI settings file {}: {e}", path.display()),
            )
        })?;

        let mut section = String::new();
        let mut map: BTreeMap<String, String> = BTreeMap::new();
        for line in raw.lines() {
            let trimmed = line.trim();
            if trimmed.is_empty() || trimmed.starts_with('#') || trimmed.starts_with(';') {
                continue;
            }
            if trimmed.starts_with('[') && trimmed.ends_with(']') {
                section = trimmed[1..trimmed.len() - 1].trim().to_string();
                continue;
            }
            if let Some((k, v)) = trimmed.split_once('=') {
                let normalized_key = k.trim().replace('\\', "/");
                let full_key = if section.is_empty() {
                    normalized_key
                } else {
                    format!("{section}/{normalized_key}")
                };
                map.insert(full_key, v.trim().trim_matches('"').to_string());
            }
        }

        let mut opts = CompressionOptions::default();

        if let Some(v) = map.get("compression_options/compression/mode") {
            opts.mode = if v.parse::<i32>().unwrap_or(0) == 1 {
                CompressionMode::Size
            } else {
                CompressionMode::Quality
            };
        }
        if let Some(v) = map.get("compression_options/compression/lossless") {
            opts.lossless = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/compression/keep_metadata") {
            opts.keep_metadata = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/compression/keep_structure") {
            opts.keep_structure = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/compression/jpeg_quality") {
            if let Ok(n) = v.parse::<u32>() {
                opts.jpeg_quality = n.min(100);
            }
        }
        if let Some(v) = map.get("compression_options/compression/jpeg_chroma_subsampling") {
            if let Ok(n) = v.parse::<i32>() {
                opts.jpeg_chroma_subsampling = ChromaSubsamplingOption::from_gui_code(n);
            }
        }
        if let Some(v) = map.get("compression_options/compression/jpeg_progressive") {
            opts.jpeg_progressive = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/compression/png_quality") {
            if let Ok(n) = v.parse::<u32>() {
                opts.png_quality = n.min(100);
            }
        }
        if let Some(v) = map.get("compression_options/compression/png_optimization_level") {
            if let Ok(n) = v.parse::<u8>() {
                opts.png_optimization_level = n.min(6);
            }
        }
        if let Some(v) = map.get("compression_options/compression/webp_quality") {
            if let Ok(n) = v.parse::<u32>() {
                opts.webp_quality = n.min(100);
            }
        }
        if let Some(v) = map.get("compression_options/compression/tiff_method") {
            if let Ok(n) = v.parse::<i32>() {
                opts.tiff_method = TiffCompressionOption::from_gui_index(n);
            }
        }
        if let Some(v) = map.get("compression_options/compression/tiff_deflate_level") {
            if let Ok(n) = v.parse::<i32>() {
                opts.tiff_deflate_level = TiffDeflateLevelOption::from_gui_slider(n);
            }
        }
        if let Some(v) = map.get("compression_options/compression/max_output_size") {
            if let Ok(n) = v.parse::<u64>() {
                opts.max_output_size.value = n.max(1);
            }
        }
        if let Some(v) = map.get("compression_options/compression/max_output_size_unit") {
            if let Ok(n) = v.parse::<i32>() {
                opts.max_output_size.unit = MaxOutputSizeUnit::from_gui_index(n);
            }
        }
        if let Some(v) = map.get("compression_options/resize/fit_to") {
            if let Ok(n) = v.parse::<i32>() {
                opts.resize.mode = ResizeMode::from_gui_index(n);
            }
        }
        if let Some(v) = map.get("compression_options/resize/width") {
            if let Ok(n) = v.parse::<u32>() {
                opts.resize.width = n.max(1);
            }
        }
        if let Some(v) = map.get("compression_options/resize/height") {
            if let Ok(n) = v.parse::<u32>() {
                opts.resize.height = n.max(1);
            }
        }
        if let Some(v) = map.get("compression_options/resize/size") {
            if let Ok(n) = v.parse::<u32>() {
                opts.resize.edge = n.max(1);
            }
        }
        if let Some(v) = map.get("compression_options/resize/do_not_enlarge") {
            opts.resize.do_not_enlarge = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/resize/keep_aspect_ratio") {
            opts.resize.keep_aspect_ratio = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/output/output_suffix") {
            opts.suffix = v.clone();
        }
        if let Some(v) = map.get("compression_options/output/same_folder_as_input") {
            opts.same_folder_as_input = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/output/skip_if_bigger") {
            opts.skip_if_bigger = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/output/format") {
            if let Ok(n) = v.parse::<i32>() {
                opts.format = OutputFormat::from_gui_index(n);
            }
        }
        if let Some(v) = map.get("compression_options/output/move_original_file") {
            if parse_qt_bool(v) {
                let dest = map
                    .get("compression_options/output/move_original_file_destination")
                    .and_then(|d| d.parse::<i32>().ok())
                    .unwrap_or(0);
                opts.move_original = if dest == 1 {
                    MoveOriginalAction::Delete
                } else {
                    MoveOriginalAction::Trash
                };
            }
        }
        if let Some(v) = map.get("compression_options/output/keep_creation_date") {
            opts.keep_dates.creation = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/output/keep_last_modified_date") {
            opts.keep_dates.modified = parse_qt_bool(v);
        }
        if let Some(v) = map.get("compression_options/output/keep_last_access_date") {
            opts.keep_dates.accessed = parse_qt_bool(v);
        }

        opts.validate()?;
        Ok(opts)
    }
}

fn parse_qt_bool(s: &str) -> bool {
    matches!(s.trim().to_ascii_lowercase().as_str(), "true" | "1" | "yes")
}
