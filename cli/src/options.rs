use crate::error::{CaesiumCliError, CliResult, ErrorCode};
use caesium::parameters::{
    CSParameters, ChromaSubsampling, GifParameters, JpegParameters, PngParameters, TiffCompression,
    TiffDeflateLevel, TiffParameters, WebPParameters,
};
use caesium::SupportedFileTypes;
use clap::ValueEnum;
use schemars::JsonSchema;
use serde::{Deserialize, Serialize};

/// Default maximum input file size guard (500 MiB, matching Caesium GUI).
pub const DEFAULT_MAX_INPUT_BYTES: u64 = 500 * 1024 * 1024;

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum CompressionMode {
    #[default]
    Quality,
    Size,
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum OutputFormat {
    #[default]
    Same,
    #[value(name = "jpg", alias = "jpeg")]
    #[serde(rename = "jpg", alias = "jpeg")]
    Jpg,
    #[value(name = "png")]
    #[serde(rename = "png")]
    Png,
    #[value(name = "webp")]
    #[serde(rename = "webp")]
    WebP,
    #[value(name = "tiff", alias = "tif")]
    #[serde(rename = "tiff", alias = "tif")]
    Tiff,
    #[value(name = "gif")]
    #[serde(rename = "gif")]
    Gif,
}

impl OutputFormat {
    pub fn extension(self) -> Option<&'static str> {
        match self {
            OutputFormat::Same => None,
            OutputFormat::Jpg => Some("jpg"),
            OutputFormat::Png => Some("png"),
            OutputFormat::WebP => Some("webp"),
            OutputFormat::Tiff => Some("tiff"),
            OutputFormat::Gif => Some("gif"),
        }
    }

    pub fn to_caesium(self) -> Option<SupportedFileTypes> {
        match self {
            OutputFormat::Same => None,
            OutputFormat::Jpg => Some(SupportedFileTypes::Jpeg),
            OutputFormat::Png => Some(SupportedFileTypes::Png),
            OutputFormat::WebP => Some(SupportedFileTypes::WebP),
            OutputFormat::Tiff => Some(SupportedFileTypes::Tiff),
            OutputFormat::Gif => Some(SupportedFileTypes::Gif),
        }
    }

    pub fn canonical_name(self) -> &'static str {
        match self {
            OutputFormat::Same => "same",
            OutputFormat::Jpg => "jpg",
            OutputFormat::Png => "png",
            OutputFormat::WebP => "webp",
            OutputFormat::Tiff => "tiff",
            OutputFormat::Gif => "gif",
        }
    }

    pub fn from_gui_index(idx: i32) -> Self {
        match idx {
            1 => OutputFormat::Jpg,
            2 => OutputFormat::Png,
            3 => OutputFormat::WebP,
            4 => OutputFormat::Tiff,
            5 => OutputFormat::Gif,
            _ => OutputFormat::Same,
        }
    }
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum ChromaSubsamplingOption {
    #[default]
    Auto,
    #[value(name = "4:4:4", alias = "444", alias = "cs444")]
    #[serde(rename = "4:4:4", alias = "444", alias = "cs444")]
    Cs444,
    #[value(name = "4:2:2", alias = "422", alias = "cs422")]
    #[serde(rename = "4:2:2", alias = "422", alias = "cs422")]
    Cs422,
    #[value(name = "4:2:0", alias = "420", alias = "cs420")]
    #[serde(rename = "4:2:0", alias = "420", alias = "cs420")]
    Cs420,
    #[value(name = "4:1:1", alias = "411", alias = "cs411")]
    #[serde(rename = "4:1:1", alias = "411", alias = "cs411")]
    Cs411,
}

impl ChromaSubsamplingOption {
    pub fn to_caesium(self) -> ChromaSubsampling {
        match self {
            ChromaSubsamplingOption::Auto => ChromaSubsampling::Auto,
            ChromaSubsamplingOption::Cs444 => ChromaSubsampling::CS444,
            ChromaSubsamplingOption::Cs422 => ChromaSubsampling::CS422,
            ChromaSubsamplingOption::Cs420 => ChromaSubsampling::CS420,
            ChromaSubsamplingOption::Cs411 => ChromaSubsampling::CS411,
        }
    }

    pub fn from_gui_code(code: i32) -> Self {
        match code {
            444 => ChromaSubsamplingOption::Cs444,
            422 => ChromaSubsamplingOption::Cs422,
            420 => ChromaSubsamplingOption::Cs420,
            411 => ChromaSubsamplingOption::Cs411,
            _ => ChromaSubsamplingOption::Auto,
        }
    }

    pub fn label(self) -> &'static str {
        match self {
            ChromaSubsamplingOption::Auto => "Auto",
            ChromaSubsamplingOption::Cs444 => "4:4:4",
            ChromaSubsamplingOption::Cs422 => "4:2:2",
            ChromaSubsamplingOption::Cs420 => "4:2:0",
            ChromaSubsamplingOption::Cs411 => "4:1:1",
        }
    }
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum TiffCompressionOption {
    Uncompressed,
    #[default]
    Lzw,
    Deflate,
    Packbits,
}

impl TiffCompressionOption {
    pub fn to_caesium(self) -> TiffCompression {
        match self {
            TiffCompressionOption::Uncompressed => TiffCompression::Uncompressed,
            TiffCompressionOption::Lzw => TiffCompression::Lzw,
            TiffCompressionOption::Deflate => TiffCompression::Deflate,
            TiffCompressionOption::Packbits => TiffCompression::Packbits,
        }
    }

    pub fn from_gui_index(idx: i32) -> Self {
        match idx {
            0 => TiffCompressionOption::Uncompressed,
            1 => TiffCompressionOption::Lzw,
            2 => TiffCompressionOption::Deflate,
            3 => TiffCompressionOption::Packbits,
            _ => TiffCompressionOption::Lzw,
        }
    }
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum TiffDeflateLevelOption {
    #[value(alias = "1")]
    Fast,
    #[default]
    #[value(alias = "6")]
    Balanced,
    #[value(alias = "9")]
    Best,
}

impl TiffDeflateLevelOption {
    pub fn to_caesium(self) -> TiffDeflateLevel {
        match self {
            TiffDeflateLevelOption::Fast => TiffDeflateLevel::Fast,
            TiffDeflateLevelOption::Balanced => TiffDeflateLevel::Balanced,
            TiffDeflateLevelOption::Best => TiffDeflateLevel::Best,
        }
    }

    pub fn from_gui_slider(val: i32) -> Self {
        match val {
            1 => TiffDeflateLevelOption::Fast,
            2 => TiffDeflateLevelOption::Balanced,
            3 | 9 => TiffDeflateLevelOption::Best,
            6 => TiffDeflateLevelOption::Balanced,
            _ => TiffDeflateLevelOption::Balanced,
        }
    }
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum ResizeMode {
    #[default]
    None,
    Dimensions,
    Percentage,
    ShortEdge,
    LongEdge,
    FixedWidth,
    FixedHeight,
}

impl ResizeMode {
    pub fn from_gui_index(idx: i32) -> Self {
        match idx {
            1 => ResizeMode::Dimensions,
            2 => ResizeMode::Percentage,
            3 => ResizeMode::ShortEdge,
            4 => ResizeMode::LongEdge,
            5 => ResizeMode::FixedWidth,
            6 => ResizeMode::FixedHeight,
            _ => ResizeMode::None,
        }
    }

    pub fn label(self) -> &'static str {
        match self {
            ResizeMode::None => "No resize",
            ResizeMode::Dimensions => "Dimensions",
            ResizeMode::Percentage => "Percentage",
            ResizeMode::ShortEdge => "Short edge",
            ResizeMode::LongEdge => "Long edge",
            ResizeMode::FixedWidth => "Fixed width",
            ResizeMode::FixedHeight => "Fixed height",
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, JsonSchema)]
#[serde(default)]
pub struct ResizeOptions {
    pub mode: ResizeMode,
    pub width: u32,
    pub height: u32,
    pub edge: u32,
    pub do_not_enlarge: bool,
    pub keep_aspect_ratio: bool,
}

impl Default for ResizeOptions {
    fn default() -> Self {
        Self {
            mode: ResizeMode::None,
            width: 1000,
            height: 1000,
            edge: 1000,
            do_not_enlarge: true,
            keep_aspect_ratio: true,
        }
    }
}

impl ResizeOptions {
    /// Parses compact resize specs such as:
    /// - `1920x1080` (dimensions)
    /// - `50%` or `50%x75%` (percentage)
    /// - `long:1200` (long edge)
    /// - `short:600` (short edge)
    /// - `w:800` / `width:800` (fixed width)
    /// - `h:600` / `height:600` (fixed height)
    pub fn parse_spec(spec: &str) -> CliResult<Self> {
        let s = spec.trim().to_ascii_lowercase();
        if s.is_empty() || s == "none" {
            return Ok(Self::default());
        }

        let mut opts = Self::default();

        if let Some(rest) = s
            .strip_prefix("long:")
            .or_else(|| s.strip_prefix("long-edge:"))
            .or_else(|| s.strip_prefix("long_edge:"))
        {
            opts.mode = ResizeMode::LongEdge;
            opts.edge = parse_positive_u32(rest, "long edge")?;
            return Ok(opts);
        }

        if let Some(rest) = s
            .strip_prefix("short:")
            .or_else(|| s.strip_prefix("short-edge:"))
            .or_else(|| s.strip_prefix("short_edge:"))
        {
            opts.mode = ResizeMode::ShortEdge;
            opts.edge = parse_positive_u32(rest, "short edge")?;
            return Ok(opts);
        }

        if let Some(rest) = s.strip_prefix("w:").or_else(|| s.strip_prefix("width:")) {
            opts.mode = ResizeMode::FixedWidth;
            opts.width = parse_positive_u32(rest, "width")?;
            return Ok(opts);
        }

        if let Some(rest) = s.strip_prefix("h:").or_else(|| s.strip_prefix("height:")) {
            opts.mode = ResizeMode::FixedHeight;
            opts.height = parse_positive_u32(rest, "height")?;
            return Ok(opts);
        }

        if s.contains('%') {
            opts.mode = ResizeMode::Percentage;
            if let Some((w_part, h_part)) = s.split_once('x') {
                opts.width = parse_positive_u32(w_part.trim_end_matches('%'), "width %")?;
                opts.height = parse_positive_u32(h_part.trim_end_matches('%'), "height %")?;
                opts.keep_aspect_ratio = opts.width == opts.height;
            } else {
                let pct = parse_positive_u32(s.trim_end_matches('%'), "percentage")?;
                opts.width = pct;
                opts.height = pct;
                opts.keep_aspect_ratio = true;
            }
            return Ok(opts);
        }

        if let Some((w_part, h_part)) = s.split_once('x') {
            opts.mode = ResizeMode::Dimensions;
            opts.width = parse_positive_u32(w_part.trim_end_matches("px"), "width")?;
            opts.height = parse_positive_u32(h_part.trim_end_matches("px"), "height")?;
            return Ok(opts);
        }

        Err(CaesiumCliError::new(
            ErrorCode::InvalidOption,
            format!("Invalid resize spec '{spec}'"),
        )
        .with_hint(
            "Use formats like 1920x1080, 50%, 50%x75%, w:800, h:600, long:1200, or short:600",
        ))
    }

    pub fn is_active(&self) -> bool {
        self.mode != ResizeMode::None
    }
}

fn parse_positive_u32(input: &str, field: &str) -> CliResult<u32> {
    let val: u32 = input.trim().parse().map_err(|_| {
        CaesiumCliError::new(
            ErrorCode::InvalidOption,
            format!("Invalid positive integer for {field}: '{input}'"),
        )
    })?;
    if val == 0 {
        return Err(CaesiumCliError::new(
            ErrorCode::InvalidOption,
            format!("{field} must be greater than 0"),
        ));
    }
    Ok(val)
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum MaxOutputSizeUnit {
    #[default]
    Bytes,
    Kilobytes,
    Megabytes,
    Percentage,
}

impl MaxOutputSizeUnit {
    pub fn from_gui_index(idx: i32) -> Self {
        match idx {
            1 => MaxOutputSizeUnit::Kilobytes,
            2 => MaxOutputSizeUnit::Megabytes,
            3 => MaxOutputSizeUnit::Percentage,
            _ => MaxOutputSizeUnit::Bytes,
        }
    }

    pub fn label(self) -> &'static str {
        match self {
            MaxOutputSizeUnit::Bytes => "bytes",
            MaxOutputSizeUnit::Kilobytes => "KB",
            MaxOutputSizeUnit::Megabytes => "MB",
            MaxOutputSizeUnit::Percentage => "%",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, JsonSchema)]
#[serde(default)]
pub struct MaxOutputSize {
    pub value: u64,
    pub unit: MaxOutputSizeUnit,
}

impl Default for MaxOutputSize {
    fn default() -> Self {
        Self {
            value: 500,
            unit: MaxOutputSizeUnit::Kilobytes,
        }
    }
}

impl MaxOutputSize {
    /// Parses sizes like `500`, `500B`, `250KB`, `2MB`, `40%`.
    pub fn parse(input: &str) -> CliResult<Self> {
        let s = input.trim();
        if s.is_empty() {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                "Max output size cannot be empty",
            ));
        }

        let upper = s.to_ascii_uppercase();
        let (num_str, unit) = if let Some(n) = upper.strip_suffix('%') {
            (n.trim(), MaxOutputSizeUnit::Percentage)
        } else if let Some(n) = upper
            .strip_suffix("MIB")
            .or_else(|| upper.strip_suffix("MB"))
            .or_else(|| upper.strip_suffix('M'))
        {
            (n.trim(), MaxOutputSizeUnit::Megabytes)
        } else if let Some(n) = upper
            .strip_suffix("KIB")
            .or_else(|| upper.strip_suffix("KB"))
            .or_else(|| upper.strip_suffix('K'))
        {
            (n.trim(), MaxOutputSizeUnit::Kilobytes)
        } else if let Some(n) = upper
            .strip_suffix("BYTES")
            .or_else(|| upper.strip_suffix('B'))
        {
            (n.trim(), MaxOutputSizeUnit::Bytes)
        } else {
            (upper.as_str(), MaxOutputSizeUnit::Bytes)
        };

        let value: u64 = num_str.parse().map_err(|_| {
            CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!("Invalid size specification '{input}'"),
            )
            .with_hint("Examples: 500KB, 2MB, 102400B, 50%")
        })?;

        if value == 0 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                "Size value must be greater than 0",
            ));
        }

        if unit == MaxOutputSizeUnit::Percentage && value > 100 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!("Percentage max output size must be between 1 and 100, got {value}%"),
            ));
        }

        Ok(Self { value, unit })
    }

    /// Computes exact byte ceiling matching `CImage::getMaxOutputSizeInBytes`.
    pub fn to_bytes(self, original_size: u64) -> usize {
        match self.unit {
            MaxOutputSizeUnit::Bytes => self.value as usize,
            MaxOutputSizeUnit::Kilobytes => (self.value.saturating_mul(1024)) as usize,
            MaxOutputSizeUnit::Megabytes => (self.value.saturating_mul(1024 * 1024)) as usize,
            MaxOutputSizeUnit::Percentage => {
                (((original_size as f64) * (self.value as f64) / 100.0).floor() as u64).max(1)
                    as usize
            }
        }
    }
}

/// Parses an absolute byte size (e.g. `500KB`, `2MB`, `1048576`). Percentage not allowed.
pub fn parse_byte_size(input: &str) -> CliResult<u64> {
    let parsed = MaxOutputSize::parse(input)?;
    match parsed.unit {
        MaxOutputSizeUnit::Percentage => Err(CaesiumCliError::new(
            ErrorCode::InvalidOption,
            format!("Expected an absolute byte size, got percentage '{input}'"),
        )),
        _ => Ok(parsed.to_bytes(0) as u64),
    }
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum MoveOriginalAction {
    #[default]
    None,
    Trash,
    Delete,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema)]
#[serde(default)]
pub struct KeepDatesOptions {
    pub creation: bool,
    pub modified: bool,
    pub accessed: bool,
}

impl KeepDatesOptions {
    pub fn all() -> Self {
        Self {
            creation: true,
            modified: true,
            accessed: true,
        }
    }

    pub fn any(self) -> bool {
        self.creation || self.modified || self.accessed
    }
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum PostCompressionAction {
    #[default]
    None,
    OpenFolder,
    Sleep,
    Shutdown,
}

#[derive(
    Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, JsonSchema, ValueEnum,
)]
#[serde(rename_all = "snake_case")]
pub enum SizeCondition {
    GreaterThan,
    EqualTo,
    LessThan,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, JsonSchema)]
pub struct SizeFilter {
    pub condition: SizeCondition,
    pub bytes: u64,
}

#[derive(Debug, Clone, PartialEq, Eq, Default, Serialize, Deserialize, JsonSchema)]
#[serde(default)]
pub struct ImportFilters {
    /// GUI parity: "Do not import if size is <condition> <bytes>".
    pub skip_by_size: Option<SizeFilter>,
    /// Keep only files whose filename matches this regex.
    pub filename_regex: Option<String>,
}

/// Complete compression, resize, conversion, and output configuration mirroring Caesium GUI + libcaesium.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, JsonSchema)]
#[serde(default)]
pub struct CompressionOptions {
    pub mode: CompressionMode,
    pub format: OutputFormat,
    pub lossless: bool,
    pub keep_metadata: bool,
    pub keep_structure: bool,
    pub same_folder_as_input: bool,
    pub suffix: String,
    pub skip_if_bigger: bool,
    pub overwrite: bool,
    pub move_original: MoveOriginalAction,
    pub keep_dates: KeepDatesOptions,

    // Codec settings
    pub jpeg_quality: u32,
    pub jpeg_chroma_subsampling: ChromaSubsamplingOption,
    pub jpeg_progressive: bool,
    pub png_quality: u32,
    pub png_optimization_level: u8,
    pub png_force_zopfli: bool,
    pub webp_quality: u32,
    pub gif_quality: u32,
    pub tiff_method: TiffCompressionOption,
    pub tiff_deflate_level: TiffDeflateLevelOption,

    // Size-mode settings
    pub max_output_size: MaxOutputSize,
    pub return_smallest: bool,

    // Resize settings
    pub resize: ResizeOptions,

    // Post-compression action
    pub post_action: PostCompressionAction,
    pub post_exec: Option<String>,
}

impl Default for CompressionOptions {
    fn default() -> Self {
        Self {
            mode: CompressionMode::Quality,
            format: OutputFormat::Same,
            lossless: false,
            keep_metadata: true,
            keep_structure: false,
            same_folder_as_input: false,
            suffix: String::new(),
            skip_if_bigger: true,
            overwrite: false,
            move_original: MoveOriginalAction::None,
            keep_dates: KeepDatesOptions::default(),

            jpeg_quality: 80,
            jpeg_chroma_subsampling: ChromaSubsamplingOption::Auto,
            jpeg_progressive: true,
            png_quality: 80,
            png_optimization_level: 3,
            png_force_zopfli: false,
            webp_quality: 60,
            gif_quality: 80,
            tiff_method: TiffCompressionOption::Lzw,
            tiff_deflate_level: TiffDeflateLevelOption::Balanced,

            max_output_size: MaxOutputSize::default(),
            return_smallest: true,

            resize: ResizeOptions::default(),

            post_action: PostCompressionAction::None,
            post_exec: None,
        }
    }
}

impl CompressionOptions {
    pub fn validate(&self) -> CliResult<()> {
        if self.jpeg_quality > 100 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!("jpeg_quality must be 0..=100, got {}", self.jpeg_quality),
            ));
        }
        if self.png_quality > 100 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!("png_quality must be 0..=100, got {}", self.png_quality),
            ));
        }
        if self.png_optimization_level > 6 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!(
                    "png_optimization_level must be 0..=6, got {}",
                    self.png_optimization_level
                ),
            ));
        }
        if self.webp_quality > 100 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!("webp_quality must be 0..=100, got {}", self.webp_quality),
            ));
        }
        if self.gif_quality > 100 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                format!("gif_quality must be 0..=100, got {}", self.gif_quality),
            ));
        }
        if self.mode == CompressionMode::Size && self.max_output_size.value == 0 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                "max_output_size must be greater than 0 in size mode",
            ));
        }
        if self.mode == CompressionMode::Size
            && self.max_output_size.unit == MaxOutputSizeUnit::Percentage
            && self.max_output_size.value > 100
        {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                "Percentage max_output_size must be 1..=100",
            ));
        }
        Ok(())
    }

    /// Emits advisory warnings matching Caesium GUI's runtime warnings.
    pub fn warnings(&self) -> Vec<String> {
        let mut out = Vec::new();
        if self.lossless
            && self.mode == CompressionMode::Quality
            && (self.format != OutputFormat::Same || self.resize.is_active())
        {
            out.push(
                "Converting to a new format or resizing can degrade quality even when lossless mode is enabled."
                    .to_string(),
            );
        }
        if self.png_force_zopfli && !self.lossless {
            out.push(
                "png_force_zopfli only takes effect for lossless PNG optimization (enable --lossless)."
                    .to_string(),
            );
        }
        out
    }

    /// Converts into `libcaesium` native `CSParameters` with resolved target width/height.
    pub fn to_cs_parameters(&self, width: u32, height: u32) -> CSParameters {
        CSParameters {
            jpeg: JpegParameters {
                quality: self.jpeg_quality,
                chroma_subsampling: self.jpeg_chroma_subsampling.to_caesium(),
                progressive: self.jpeg_progressive,
            },
            png: PngParameters {
                quality: self.png_quality,
                force_zopfli: self.png_force_zopfli,
                optimization_level: self.png_optimization_level,
            },
            gif: GifParameters {
                quality: self.gif_quality,
            },
            webp: WebPParameters {
                quality: self.webp_quality,
            },
            tiff: TiffParameters {
                algorithm: self.tiff_method.to_caesium(),
                deflate_level: self.tiff_deflate_level.to_caesium(),
            },
            keep_metadata: self.keep_metadata,
            optimize: self.lossless,
            width,
            height,
        }
    }

    /// Applies a single uniform quality value across all lossy codecs.
    pub fn apply_universal_quality(&mut self, q: u32) {
        self.jpeg_quality = q;
        self.png_quality = q;
        self.webp_quality = q;
        self.gif_quality = q;
    }
}

/// Declarative multi-task batch job specification for agents and CI pipelines.
#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct BatchJobSpec {
    #[serde(default)]
    pub threads: Option<usize>,
    #[serde(default)]
    pub dry_run: bool,
    pub tasks: Vec<BatchTaskSpec>,
}

#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct BatchTaskSpec {
    #[serde(default)]
    pub name: Option<String>,
    pub inputs: Vec<String>,
    #[serde(default)]
    pub output: Option<String>,
    #[serde(default)]
    pub preset: Option<String>,
    #[serde(default = "default_true")]
    pub recursive: bool,
    #[serde(default)]
    pub filters: ImportFilters,
    #[serde(default)]
    pub options: CompressionOptions,
}

fn default_true() -> bool {
    true
}

/// Formats byte count into human-readable string matching Caesium GUI `toHumanSize`.
pub fn format_human_size(bytes: i64) -> String {
    let negative = bytes < 0;
    let abs = bytes.unsigned_abs() as f64;
    if abs == 0.0 {
        return "0 bytes".to_string();
    }
    const UNITS: [&str; 5] = ["bytes", "KB", "MB", "GB", "TB"];
    let order = ((abs.log2() / 10.0).floor() as usize).min(4);
    let scaled = abs / 1024_f64.powi(order as i32);
    let sign = if negative { "-" } else { "" };
    format!("{sign}{scaled:.2} {}", UNITS[order])
}
