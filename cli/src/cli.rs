use crate::agent::AgentSession;
use crate::config::{ConfigStore, PresetStore};
use crate::engine::{
    compress_bytes, dry_run_batch, preview_file, retry_from_receipt, run_batch_with_progress,
    ItemResult, JobReceipt, OutputTarget,
};
use crate::error::{exit_codes, CaesiumCliError, CliResult, ErrorCode};
use crate::inspect::{inspect_bytes, inspect_path};
use crate::mcp::{execute_batch_spec, McpServer};
use crate::options::{
    parse_byte_size, BatchJobSpec, ChromaSubsamplingOption, CompressionMode, CompressionOptions,
    ImportFilters, KeepDatesOptions, MaxOutputSize, MoveOriginalAction, OutputFormat,
    PostCompressionAction, ResizeMode, ResizeOptions, SizeCondition, SizeFilter,
    TiffCompressionOption, TiffDeflateLevelOption, DEFAULT_MAX_INPUT_BYTES,
};
use crate::output::{
    capabilities_payload, error_envelope, format_human_dry_run, format_human_inspect,
    format_human_presets, format_human_previews, format_human_report, print_json,
    print_ndjson_line, schema_for_target, JsonEnvelope, OutputStyle,
};
use crate::post_action::run_post_action;
use crate::scanner::{load_paths_from_list, load_paths_from_reader, scan_inputs, ScanOptions};
use crate::tui;
use crate::{APP_NAME, APP_VERSION, SCHEMA_VERSION};
use clap::{Args, CommandFactory, Parser, Subcommand};
use clap_complete::{generate, Shell};
use serde_json::{json, Value};
use std::fs;
use std::io::{self, IsTerminal, Read, Write};
use std::path::PathBuf;
use std::sync::Mutex;

#[derive(Parser, Debug)]
#[command(
    name = APP_NAME,
    version = APP_VERSION,
    about = "Caesium Image Compressor — extensive CLI, interactive TUI, and agent/MCP server",
    long_about = "Lossy and lossless image compression (JPEG, PNG, WebP, TIFF, GIF) powered by libcaesium.\n\
                  Supports one-shot CLI operations, stdin/stdout streaming, interactive TUI,\n\
                  machine-readable JSON/NDJSON output, stateful agent sessions, and native MCP over stdio."
)]
pub struct Cli {
    /// Output format for command results
    #[arg(long, global = true, value_enum, default_value_t = OutputStyle::Human)]
    pub output_format: OutputStyle,

    /// Emit machine-readable JSON envelope (shorthand for --output-format json)
    #[arg(long, global = true)]
    pub json: bool,

    /// Stream newline-delimited JSON events (shorthand for --output-format ndjson)
    #[arg(long, global = true)]
    pub ndjson: bool,

    /// Suppress non-essential output (shorthand for --output-format quiet)
    #[arg(long, global = true)]
    pub quiet: bool,

    #[command(subcommand)]
    pub command: Option<Commands>,
}

impl Cli {
    pub fn effective_output_style(&self) -> OutputStyle {
        if self.ndjson {
            OutputStyle::Ndjson
        } else if self.json {
            OutputStyle::Json
        } else if self.quiet {
            OutputStyle::Quiet
        } else {
            self.output_format
        }
    }
}

#[derive(Subcommand, Debug)]
pub enum Commands {
    /// Compress images or directories (full Caesium GUI parity + stdin/stdout streaming)
    #[command(alias = "c")]
    Compress(CompressArgs),

    /// Convert images between JPG, PNG, WebP, TIFF, and GIF with compression
    #[command(alias = "conv")]
    Convert(ConvertArgs),

    /// Inspect image metadata, format, EXIF orientation, dimensions, and SHA-256 hash
    #[command(alias = "info")]
    Inspect(InspectArgs),

    /// Preview exact compressed size and savings in memory without writing output files
    Preview(PreviewArgs),

    /// Execute a declarative multi-task JSON batch manifest (file path or '-' for stdin)
    Batch(BatchArgs),

    /// Retry only failed items from a saved job receipt JSON file
    Retry(RetryArgs),

    /// Launch the interactive Terminal User Interface (TUI)
    Tui(TuiArgs),

    /// Start a stateful NDJSON interactive session over stdin/stdout for AI agents
    Agent,

    /// Start a native Model Context Protocol (MCP) JSON-RPC 2.0 server over stdio
    Mcp,

    /// Manage built-in and custom compression presets
    Preset {
        #[command(subcommand)]
        action: PresetCommands,
    },

    /// View, reset, or import Caesium configuration (including desktop GUI QSettings INI)
    Config {
        #[command(subcommand)]
        action: ConfigCommands,
    },

    /// Print Draft 2020-12 JSON Schema for CLI/agent data structures
    Schema {
        /// Schema target: options, batch, report, inspect, preview, dry-run, receipt, all
        #[arg(default_value = "options")]
        target: String,
    },

    /// Print complete machine-readable capabilities, codecs, protocols, and exit codes
    Capabilities,

    /// Verify libcaesium codecs, runtime environment, and configuration health
    Doctor,

    /// Generate shell completion scripts
    Completions {
        #[arg(value_enum)]
        shell: Shell,
    },
}

/// Shared compression, codec, resize, and metadata tuning flags.
#[derive(Args, Debug, Clone, Default)]
pub struct TuningArgs {
    /// Apply a named preset before CLI overrides (see `preset list`)
    #[arg(long)]
    pub preset: Option<String>,

    /// Set quality (0-100) for all lossy codecs (overridden by format-specific quality flags)
    #[arg(short = 'q', long)]
    pub quality: Option<u32>,

    /// Target maximum output file size (switches to size mode), e.g. 250KB, 2MB, 50%
    #[arg(long)]
    pub max_size: Option<String>,

    /// Fail in size mode if the target size cannot be met (default returns smallest possible)
    #[arg(long)]
    pub strict_size: bool,

    /// Enable lossless optimization
    #[arg(long)]
    pub lossless: bool,

    /// Preserve EXIF/ICC metadata (default: true)
    #[arg(long, conflicts_with = "strip_metadata")]
    pub keep_metadata: bool,

    /// Strip EXIF/ICC metadata
    #[arg(long, conflicts_with = "keep_metadata")]
    pub strip_metadata: bool,

    /// Output format (same, jpg, png, webp, tiff, gif)
    #[arg(short = 'f', long, value_enum)]
    pub format: Option<OutputFormat>,

    /// JPEG quality (0-100, default 80)
    #[arg(long)]
    pub jpeg_quality: Option<u32>,

    /// JPEG chroma subsampling (auto, 4:4:4, 4:2:2, 4:2:0, 4:1:1)
    #[arg(long, value_enum)]
    pub jpeg_chroma: Option<ChromaSubsamplingOption>,

    /// Enable progressive JPEG encoding (default: true)
    #[arg(long, conflicts_with = "no_jpeg_progressive")]
    pub jpeg_progressive: bool,

    /// Disable progressive JPEG encoding
    #[arg(long, conflicts_with = "jpeg_progressive")]
    pub no_jpeg_progressive: bool,

    /// PNG lossy quality (0-100, default 80)
    #[arg(long)]
    pub png_quality: Option<u32>,

    /// PNG lossless optimization level (0-6, default 3)
    #[arg(long)]
    pub png_opt_level: Option<u8>,

    /// Force Zopfli PNG deflate (slower, smaller lossless PNGs)
    #[arg(long)]
    pub png_zopfli: bool,

    /// WebP quality (0-100, default 60)
    #[arg(long)]
    pub webp_quality: Option<u32>,

    /// GIF quality (0-100, default 80)
    #[arg(long)]
    pub gif_quality: Option<u32>,

    /// TIFF compression algorithm (uncompressed, lzw, deflate, packbits)
    #[arg(long, value_enum)]
    pub tiff_method: Option<TiffCompressionOption>,

    /// TIFF deflate level (fast, balanced, best)
    #[arg(long, value_enum)]
    pub tiff_deflate_level: Option<TiffDeflateLevelOption>,

    /// Compact resize spec: 1920x1080, 50%, 50%x75%, w:800, h:600, long:1200, short:600
    #[arg(long)]
    pub resize: Option<String>,

    /// Target width in pixels (combined with --height sets Dimensions, alone sets FixedWidth)
    #[arg(long)]
    pub width: Option<u32>,

    /// Target height in pixels (combined with --width sets Dimensions, alone sets FixedHeight)
    #[arg(long)]
    pub height: Option<u32>,

    /// Target long edge in pixels
    #[arg(long)]
    pub long_edge: Option<u32>,

    /// Target short edge in pixels
    #[arg(long)]
    pub short_edge: Option<u32>,

    /// Resize percentage (1-999)
    #[arg(long)]
    pub resize_percent: Option<u32>,

    /// Do not enlarge images smaller than target dimensions (default: true)
    #[arg(long, conflicts_with = "allow_enlarge")]
    pub do_not_enlarge: bool,

    /// Allow upscaling images smaller than target dimensions
    #[arg(long, conflicts_with = "do_not_enlarge")]
    pub allow_enlarge: bool,
}

/// Shared discovery & Advanced Import filter flags.
#[derive(Args, Debug, Clone, Default)]
pub struct FilterArgs {
    /// Read additional input paths (one per line) from file or '-' for stdin
    #[arg(long)]
    pub from_list: Option<String>,

    /// Recurse into subdirectories (default: true)
    #[arg(short = 'r', long, conflicts_with = "no_recursive")]
    pub recursive: bool,

    /// Do not recurse into subdirectories
    #[arg(long, conflicts_with = "recursive")]
    pub no_recursive: bool,

    /// Max input file size guard (e.g. 500MB, or 0 for unlimited)
    #[arg(long)]
    pub max_input_size: Option<String>,

    /// Skip files whose size is greater than this threshold (e.g. 10MB)
    #[arg(long, conflicts_with_all = ["skip_size_eq", "skip_size_lt"])]
    pub skip_size_gt: Option<String>,

    /// Skip files whose size equals this threshold
    #[arg(long, conflicts_with_all = ["skip_size_gt", "skip_size_lt"])]
    pub skip_size_eq: Option<String>,

    /// Skip files whose size is less than this threshold (e.g. 50KB)
    #[arg(long, conflicts_with_all = ["skip_size_gt", "skip_size_eq"])]
    pub skip_size_lt: Option<String>,

    /// Keep only files whose filename matches this regular expression
    #[arg(long)]
    pub regex: Option<String>,
}

/// Shared output & execution flags.
#[derive(Args, Debug, Clone, Default)]
pub struct OutputArgs {
    /// Output directory, single output file, or '-' for stdout streaming
    #[arg(short = 'o', long)]
    pub output: Option<String>,

    /// Write compressed outputs into the same folder as each input
    #[arg(long)]
    pub same_folder: bool,

    /// Append a suffix before the file extension (e.g. _min)
    #[arg(long)]
    pub suffix: Option<String>,

    /// Preserve relative subdirectory structure inside --output
    #[arg(long)]
    pub keep_structure: bool,

    /// Allow overwriting original input files in-place
    #[arg(long, short = 'y')]
    pub overwrite: bool,

    /// Skip writing compressed output if it is bigger than the original (default: true)
    #[arg(long, conflicts_with = "no_skip_if_bigger")]
    pub skip_if_bigger: bool,

    /// Always write compressed output even if bigger than the original
    #[arg(long, conflicts_with = "skip_if_bigger")]
    pub no_skip_if_bigger: bool,

    /// Action to take on original file after successful compression (none, trash, delete)
    #[arg(long, value_enum)]
    pub move_original: Option<MoveOriginalAction>,

    /// Preserve all supported file timestamps (modified + accessed)
    #[arg(long)]
    pub keep_dates: bool,

    /// Preserve last-modified file timestamp
    #[arg(long)]
    pub keep_modified: bool,

    /// Preserve last-accessed file timestamp
    #[arg(long)]
    pub keep_accessed: bool,

    /// Worker thread count (0 = auto-detect CPU cores)
    #[arg(short = 'j', long)]
    pub threads: Option<usize>,

    /// Plan operations without writing or modifying any files
    #[arg(long)]
    pub dry_run: bool,

    /// Save a job receipt JSON file (can be passed to `caesium-cli retry`)
    #[arg(long)]
    pub receipt: Option<PathBuf>,

    /// Post-compression action (none, open-folder, sleep, shutdown)
    #[arg(long, value_enum)]
    pub post_action: Option<PostCompressionAction>,

    /// Shell command to execute after successful compression
    #[arg(long = "exec")]
    pub post_exec: Option<String>,
}

#[derive(Args, Debug)]
pub struct CompressArgs {
    /// Input image files, directories, or '-' for stdin
    pub inputs: Vec<String>,

    #[command(flatten)]
    pub tuning: TuningArgs,

    #[command(flatten)]
    pub filters: FilterArgs,

    #[command(flatten)]
    pub out: OutputArgs,
}

#[derive(Args, Debug)]
pub struct ConvertArgs {
    /// Input image files, directories, or '-' for stdin
    pub inputs: Vec<String>,

    /// Target image format to convert to
    #[arg(long, value_enum)]
    pub to: OutputFormat,

    #[command(flatten)]
    pub tuning: TuningArgs,

    #[command(flatten)]
    pub filters: FilterArgs,

    #[command(flatten)]
    pub out: OutputArgs,
}

#[derive(Args, Debug)]
pub struct InspectArgs {
    /// Input image files, directories, or '-' for stdin
    pub paths: Vec<String>,

    #[command(flatten)]
    pub filters: FilterArgs,
}

#[derive(Args, Debug)]
pub struct PreviewArgs {
    /// Input image files or directories to preview in memory
    pub paths: Vec<String>,

    #[command(flatten)]
    pub tuning: TuningArgs,

    #[command(flatten)]
    pub filters: FilterArgs,

    /// Optional path to write the single preview image bytes
    #[arg(long)]
    pub write_preview: Option<PathBuf>,
}

#[derive(Args, Debug)]
pub struct BatchArgs {
    /// Path to JSON BatchJobSpec file, or '-' to read from stdin
    pub spec: String,

    /// Override worker threads
    #[arg(short = 'j', long)]
    pub threads: Option<usize>,

    /// Force dry-run across all tasks in the manifest
    #[arg(long)]
    pub dry_run: bool,
}

#[derive(Args, Debug)]
pub struct RetryArgs {
    /// Path to a saved job receipt JSON file
    pub receipt: PathBuf,

    /// Worker thread count (0 = auto)
    #[arg(short = 'j', long, default_value_t = 0)]
    pub threads: usize,
}

#[derive(Args, Debug)]
pub struct TuiArgs {
    /// Optional files or directories to pre-load into the TUI queue
    pub paths: Vec<PathBuf>,

    /// Output folder pre-filled in the Output tab
    #[arg(short = 'o', long)]
    pub output: Option<PathBuf>,

    #[command(flatten)]
    pub tuning: TuningArgs,
}

#[derive(Subcommand, Debug)]
pub enum PresetCommands {
    /// List all built-in and custom presets
    List,
    /// Show full JSON options for one preset
    Show { name: String },
    /// Save a custom preset from CLI flags
    Save {
        name: String,
        #[arg(long)]
        description: Option<String>,
        #[command(flatten)]
        tuning: TuningArgs,
    },
    /// Delete a custom preset
    Delete { name: String },
    /// Export a preset as standalone JSON
    Export { name: String },
    /// Import a preset JSON file
    Import {
        file: PathBuf,
        #[arg(long)]
        name: Option<String>,
    },
}

#[derive(Subcommand, Debug)]
pub enum ConfigCommands {
    /// Show current CLI default configuration
    Show,
    /// Show configuration file path
    Path,
    /// Reset configuration to Caesium defaults
    Reset,
    /// Import compression defaults from Caesium desktop GUI's Qt QSettings INI file
    ImportGui {
        /// Optional custom path to Caesium Image Compressor.conf
        #[arg(long)]
        path: Option<PathBuf>,
    },
}

/// Top-level CLI runner returning a deterministic process exit code.
pub fn run_cli() -> i32 {
    let cli = Cli::parse();
    let style = cli.effective_output_style();
    let cmd_name = command_label(&cli.command);

    match dispatch(cli, style) {
        Ok(code) => code,
        Err(err) => {
            match style {
                OutputStyle::Json => print_json(&error_envelope(cmd_name, &err)),
                OutputStyle::Ndjson => print_ndjson_line(&error_envelope(cmd_name, &err)),
                OutputStyle::Human => eprintln!("Error: {err}"),
                OutputStyle::Quiet => {}
            }
            err.exit_code()
        }
    }
}

fn command_label(cmd: &Option<Commands>) -> &'static str {
    match cmd {
        None => "tui",
        Some(Commands::Compress(_)) => "compress",
        Some(Commands::Convert(_)) => "convert",
        Some(Commands::Inspect(_)) => "inspect",
        Some(Commands::Preview(_)) => "preview",
        Some(Commands::Batch(_)) => "batch",
        Some(Commands::Retry(_)) => "retry",
        Some(Commands::Tui(_)) => "tui",
        Some(Commands::Agent) => "agent",
        Some(Commands::Mcp) => "mcp",
        Some(Commands::Preset { .. }) => "preset",
        Some(Commands::Config { .. }) => "config",
        Some(Commands::Schema { .. }) => "schema",
        Some(Commands::Capabilities) => "capabilities",
        Some(Commands::Doctor) => "doctor",
        Some(Commands::Completions { .. }) => "completions",
    }
}

fn dispatch(cli: Cli, style: OutputStyle) -> CliResult<i32> {
    let config_store = ConfigStore::default_store();
    let preset_store = PresetStore::default_store();

    match cli.command {
        None => {
            if io::stdin().is_terminal() && io::stdout().is_terminal() {
                let cfg = config_store.load().unwrap_or_default();
                tui::run_tui(Vec::new(), cfg.defaults, None)?;
                Ok(exit_codes::SUCCESS)
            } else {
                let mut cmd = Cli::command();
                cmd.print_help()?;
                println!();
                Ok(exit_codes::SUCCESS)
            }
        }
        Some(Commands::Compress(args)) => {
            handle_compress(args, None, style, &config_store, &preset_store)
        }
        Some(Commands::Convert(args)) => {
            let to = args.to;
            if to == OutputFormat::Same {
                return Err(CaesiumCliError::new(
                    ErrorCode::InvalidOption,
                    "convert --to requires a concrete format (jpg, png, webp, tiff, or gif)",
                ));
            }
            let cargs = CompressArgs {
                inputs: args.inputs,
                tuning: args.tuning,
                filters: args.filters,
                out: args.out,
            };
            handle_compress(cargs, Some(to), style, &config_store, &preset_store)
        }
        Some(Commands::Inspect(args)) => handle_inspect(args, style, &config_store),
        Some(Commands::Preview(args)) => handle_preview(args, style, &config_store, &preset_store),
        Some(Commands::Batch(args)) => handle_batch(args, style, &preset_store),
        Some(Commands::Retry(args)) => handle_retry(args, style),
        Some(Commands::Tui(args)) => {
            let cfg = config_store.load().unwrap_or_default();
            let opts = build_options(&cfg.defaults, &args.tuning, None, &preset_store, None)?;
            tui::run_tui(args.paths, opts, args.output)?;
            Ok(exit_codes::SUCCESS)
        }
        Some(Commands::Agent) => {
            let mut session = AgentSession::new();
            session.run_stdio()?;
            Ok(exit_codes::SUCCESS)
        }
        Some(Commands::Mcp) => {
            let mut server = McpServer::new();
            server.run_stdio()?;
            Ok(exit_codes::SUCCESS)
        }
        Some(Commands::Preset { action }) => {
            handle_preset(action, style, &config_store, &preset_store)
        }
        Some(Commands::Config { action }) => handle_config(action, style, &config_store),
        Some(Commands::Schema { target }) => {
            let schema = schema_for_target(&target)?;
            print_json(&schema);
            Ok(exit_codes::SUCCESS)
        }
        Some(Commands::Capabilities) => {
            let payload = capabilities_payload();
            match style {
                OutputStyle::Json => {
                    print_json(&JsonEnvelope::ok("capabilities", payload, vec![]));
                }
                OutputStyle::Ndjson => {
                    print_ndjson_line(
                        &json!({ "ok": true, "command": "capabilities", "result": payload }),
                    );
                }
                OutputStyle::Human => {
                    print_json(&payload);
                }
                OutputStyle::Quiet => {}
            }
            Ok(exit_codes::SUCCESS)
        }
        Some(Commands::Doctor) => handle_doctor(style, &config_store),
        Some(Commands::Completions { shell }) => {
            let mut cmd = Cli::command();
            let mut buf = Vec::new();
            generate(shell, &mut cmd, APP_NAME, &mut buf);
            if let Err(e) = io::stdout().write_all(&buf) {
                if e.kind() != io::ErrorKind::BrokenPipe {
                    return Err(e.into());
                }
            }
            Ok(exit_codes::SUCCESS)
        }
    }
}

fn build_options(
    base: &CompressionOptions,
    tuning: &TuningArgs,
    out: Option<&OutputArgs>,
    presets: &PresetStore,
    forced_format: Option<OutputFormat>,
) -> CliResult<CompressionOptions> {
    let mut opts = if let Some(preset_name) = &tuning.preset {
        presets.get(preset_name)?.options
    } else {
        base.clone()
    };

    if let Some(q) = tuning.quality {
        opts.apply_universal_quality(q);
    }
    if let Some(ms) = &tuning.max_size {
        opts.mode = CompressionMode::Size;
        opts.max_output_size = MaxOutputSize::parse(ms)?;
    }
    if tuning.strict_size {
        opts.return_smallest = false;
    }
    if tuning.lossless {
        opts.lossless = true;
    }
    if tuning.keep_metadata {
        opts.keep_metadata = true;
    } else if tuning.strip_metadata {
        opts.keep_metadata = false;
    }
    if let Some(fmt) = forced_format.or(tuning.format) {
        opts.format = fmt;
    }
    if let Some(q) = tuning.jpeg_quality {
        opts.jpeg_quality = q;
    }
    if let Some(ch) = tuning.jpeg_chroma {
        opts.jpeg_chroma_subsampling = ch;
    }
    if tuning.jpeg_progressive {
        opts.jpeg_progressive = true;
    } else if tuning.no_jpeg_progressive {
        opts.jpeg_progressive = false;
    }
    if let Some(q) = tuning.png_quality {
        opts.png_quality = q;
    }
    if let Some(lvl) = tuning.png_opt_level {
        opts.png_optimization_level = lvl;
    }
    if tuning.png_zopfli {
        opts.png_force_zopfli = true;
    }
    if let Some(q) = tuning.webp_quality {
        opts.webp_quality = q;
    }
    if let Some(q) = tuning.gif_quality {
        opts.gif_quality = q;
    }
    if let Some(m) = tuning.tiff_method {
        opts.tiff_method = m;
    }
    if let Some(dl) = tuning.tiff_deflate_level {
        opts.tiff_deflate_level = dl;
    }

    // Resize flags
    if let Some(spec) = &tuning.resize {
        opts.resize = ResizeOptions::parse_spec(spec)?;
    } else if let Some(edge) = tuning.long_edge {
        opts.resize.mode = ResizeMode::LongEdge;
        opts.resize.edge = edge;
    } else if let Some(edge) = tuning.short_edge {
        opts.resize.mode = ResizeMode::ShortEdge;
        opts.resize.edge = edge;
    } else if let Some(pct) = tuning.resize_percent {
        opts.resize.mode = ResizeMode::Percentage;
        opts.resize.width = pct;
        opts.resize.height = pct;
    } else if let (Some(w), Some(h)) = (tuning.width, tuning.height) {
        opts.resize.mode = ResizeMode::Dimensions;
        opts.resize.width = w;
        opts.resize.height = h;
    } else if let Some(w) = tuning.width {
        opts.resize.mode = ResizeMode::FixedWidth;
        opts.resize.width = w;
    } else if let Some(h) = tuning.height {
        opts.resize.mode = ResizeMode::FixedHeight;
        opts.resize.height = h;
    }

    if tuning.do_not_enlarge {
        opts.resize.do_not_enlarge = true;
    } else if tuning.allow_enlarge {
        opts.resize.do_not_enlarge = false;
    }

    // Output flags
    if let Some(out_args) = out {
        if out_args.same_folder {
            opts.same_folder_as_input = true;
        }
        if let Some(s) = &out_args.suffix {
            opts.suffix = s.clone();
        }
        if out_args.keep_structure {
            opts.keep_structure = true;
        }
        if out_args.overwrite {
            opts.overwrite = true;
        }
        if out_args.skip_if_bigger {
            opts.skip_if_bigger = true;
        } else if out_args.no_skip_if_bigger {
            opts.skip_if_bigger = false;
        } else if forced_format.is_some() {
            // `convert` defaults to writing the converted format even if larger
            opts.skip_if_bigger = false;
        }
        if let Some(mv) = out_args.move_original {
            opts.move_original = mv;
        }
        if out_args.keep_dates {
            opts.keep_dates = KeepDatesOptions::all();
        } else {
            if out_args.keep_modified {
                opts.keep_dates.modified = true;
            }
            if out_args.keep_accessed {
                opts.keep_dates.accessed = true;
            }
        }
        if let Some(pa) = out_args.post_action {
            opts.post_action = pa;
        }
        if let Some(ex) = &out_args.post_exec {
            opts.post_exec = Some(ex.clone());
        }
    }

    opts.validate()?;
    Ok(opts)
}

fn build_scan_options(
    filters: &FilterArgs,
    default_recursive: bool,
    default_max: u64,
) -> CliResult<ScanOptions> {
    let recursive = if filters.recursive {
        true
    } else if filters.no_recursive {
        false
    } else {
        default_recursive
    };

    let max_input_bytes = match &filters.max_input_size {
        Some(s) if s.trim() == "0" => None,
        Some(s) => Some(parse_byte_size(s)?),
        None => Some(default_max),
    };

    let skip_by_size = if let Some(s) = &filters.skip_size_gt {
        Some(SizeFilter {
            condition: SizeCondition::GreaterThan,
            bytes: parse_byte_size(s)?,
        })
    } else if let Some(s) = &filters.skip_size_eq {
        Some(SizeFilter {
            condition: SizeCondition::EqualTo,
            bytes: parse_byte_size(s)?,
        })
    } else if let Some(s) = &filters.skip_size_lt {
        Some(SizeFilter {
            condition: SizeCondition::LessThan,
            bytes: parse_byte_size(s)?,
        })
    } else {
        None
    };

    Ok(ScanOptions {
        recursive,
        filters: ImportFilters {
            skip_by_size,
            filename_regex: filters.regex.clone(),
        },
        max_input_bytes,
    })
}

fn collect_input_paths(raw_inputs: &[String], from_list: Option<&str>) -> CliResult<Vec<PathBuf>> {
    let mut paths: Vec<PathBuf> = raw_inputs.iter().map(PathBuf::from).collect();
    if let Some(list_src) = from_list {
        let extra = if list_src == "-" {
            load_paths_from_reader(io::stdin().lock())?
        } else {
            load_paths_from_list(&PathBuf::from(list_src))?
        };
        paths.extend(extra);
    }
    Ok(paths)
}

fn handle_compress(
    args: CompressArgs,
    forced_format: Option<OutputFormat>,
    style: OutputStyle,
    config_store: &ConfigStore,
    preset_store: &PresetStore,
) -> CliResult<i32> {
    let cfg = config_store.load().unwrap_or_default();
    let opts = build_options(
        &cfg.defaults,
        &args.tuning,
        Some(&args.out),
        preset_store,
        forced_format,
    )?;

    // Stdin/stdout streaming mode: `caesium-cli compress - -o -` or `-o out.webp`
    if args.inputs.len() == 1 && args.inputs[0] == "-" {
        let mut buf = Vec::new();
        io::stdin().read_to_end(&mut buf)?;
        let res = compress_bytes(&buf, &opts)?;
        match args.out.output.as_deref() {
            None | Some("-") => {
                let mut stdout = io::stdout().lock();
                stdout.write_all(&res.bytes)?;
                stdout.flush()?;
            }
            Some(path) => {
                fs::write(path, &res.bytes)?;
                if style == OutputStyle::Json {
                    print_json(&JsonEnvelope::ok(
                        "compress",
                        json!({
                            "output_path": path,
                            "input_format": res.input_format,
                            "output_format": res.output_format,
                            "original_size": res.original_size,
                            "compressed_size": res.compressed_size,
                            "saved_bytes": res.saved_bytes,
                            "saved_percent": res.saved_percent,
                        }),
                        res.warnings,
                    ));
                }
            }
        }
        return Ok(exit_codes::SUCCESS);
    }

    let raw_paths = collect_input_paths(&args.inputs, args.filters.from_list.as_deref())?;
    if raw_paths.is_empty() {
        return Err(CaesiumCliError::new(
            ErrorCode::NoMatchingInputs,
            "No input files or directories provided",
        )
        .with_hint("Pass file/folder paths, `--from-list <file>`, or `-` for stdin"));
    }

    let scan_opts =
        build_scan_options(&args.filters, cfg.recursive_by_default, cfg.max_input_bytes)?;
    let scan = scan_inputs(&raw_paths, &scan_opts)?;
    if scan.files.is_empty() {
        return Err(CaesiumCliError::new(
            ErrorCode::NoMatchingInputs,
            "No supported images matched the inputs and filters",
        ));
    }

    let target = if let Some(out_str) = &args.out.output {
        let p = PathBuf::from(out_str);
        if scan.files.len() == 1
            && !p.is_dir()
            && p.extension().is_some()
            && !out_str.ends_with(std::path::MAIN_SEPARATOR)
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
            "Output destination is required",
        )
        .with_hint("Specify `-o <DIR_OR_FILE>` or `--same-folder`"));
    };

    let cmd_name = if forced_format.is_some() {
        "convert"
    } else {
        "compress"
    };

    if args.out.dry_run {
        let plan = dry_run_batch(&scan.files, target, &opts, Some(scan.root_folder))?;
        match style {
            OutputStyle::Json => {
                print_json(&JsonEnvelope::ok(cmd_name, &plan, plan.warnings.clone()))
            }
            OutputStyle::Ndjson => {
                print_ndjson_line(&json!({ "event": "dry_run", "plan": plan }));
            }
            OutputStyle::Human => println!("{}", format_human_dry_run(&plan)),
            OutputStyle::Quiet => {}
        }
        return Ok(exit_codes::SUCCESS);
    }

    let threads = args.out.threads.or(cfg.default_threads).unwrap_or(0);

    if style == OutputStyle::Ndjson {
        print_ndjson_line(&json!({
            "event": "start",
            "schema_version": SCHEMA_VERSION,
            "command": cmd_name,
            "total": scan.files.len(),
            "root_folder": scan.root_folder,
        }));
    }

    let ndjson_lock = Mutex::new(());
    let report = run_batch_with_progress(
        &scan.files,
        target.clone(),
        &opts,
        threads,
        Some(scan.root_folder),
        None,
        |item: &ItemResult| {
            if style == OutputStyle::Ndjson {
                let _g = ndjson_lock.lock();
                print_ndjson_line(&json!({
                    "event": "item",
                    "item": item,
                }));
            }
        },
    )?;

    if let Some(receipt_path) = &args.out.receipt {
        let receipt = JobReceipt::from_report(&report, target.clone(), &opts);
        receipt.save(receipt_path)?;
    }

    match style {
        OutputStyle::Json => {
            print_json(&JsonEnvelope::ok(
                cmd_name,
                &report,
                report.warnings.clone(),
            ));
        }
        OutputStyle::Ndjson => {
            print_ndjson_line(&json!({
                "event": "complete",
                "summary": report.summary,
                "warnings": report.warnings,
            }));
        }
        OutputStyle::Human => {
            println!("{}", format_human_report(&report));
        }
        OutputStyle::Quiet => {}
    }

    if report.summary.failed == 0 {
        let folder = match &target {
            OutputTarget::Directory(d) => Some(d.as_path()),
            OutputTarget::SingleFile(f) => f.parent(),
            OutputTarget::SameFolder => Some(report.root_folder.as_path()),
        };
        run_post_action(opts.post_action, folder, opts.post_exec.as_deref())?;
        Ok(exit_codes::SUCCESS)
    } else {
        Ok(exit_codes::PARTIAL_FAILURE)
    }
}

fn handle_inspect(
    args: InspectArgs,
    style: OutputStyle,
    config_store: &ConfigStore,
) -> CliResult<i32> {
    if args.paths.len() == 1 && args.paths[0] == "-" {
        let mut buf = Vec::new();
        io::stdin().read_to_end(&mut buf)?;
        let info = inspect_bytes(&buf, Some("<stdin>"))?;
        let items = vec![info];
        emit_inspect(&items, Vec::new(), style);
        return Ok(exit_codes::SUCCESS);
    }

    let cfg = config_store.load().unwrap_or_default();
    let raw_paths = collect_input_paths(&args.paths, args.filters.from_list.as_deref())?;
    let scan_opts = build_scan_options(&args.filters, cfg.recursive_by_default, 0)?;
    let scan = scan_inputs(&raw_paths, &scan_opts)?;
    if scan.files.is_empty() {
        return Err(CaesiumCliError::new(
            ErrorCode::NoMatchingInputs,
            "No matching images found to inspect",
        ));
    }

    let mut items = Vec::with_capacity(scan.files.len());
    for f in &scan.files {
        items.push(inspect_path(f)?);
    }
    emit_inspect(&items, scan.skipped, style);
    Ok(exit_codes::SUCCESS)
}

fn emit_inspect(
    items: &[crate::inspect::ImageInfo],
    skipped: Vec<crate::scanner::SkippedInput>,
    style: OutputStyle,
) {
    match style {
        OutputStyle::Json => {
            print_json(&JsonEnvelope::ok(
                "inspect",
                json!({ "items": items, "skipped": skipped }),
                vec![],
            ));
        }
        OutputStyle::Ndjson => {
            for item in items {
                print_ndjson_line(&json!({ "event": "inspect", "item": item }));
            }
        }
        OutputStyle::Human => println!("{}", format_human_inspect(items)),
        OutputStyle::Quiet => {}
    }
}

fn handle_preview(
    args: PreviewArgs,
    style: OutputStyle,
    config_store: &ConfigStore,
    preset_store: &PresetStore,
) -> CliResult<i32> {
    let cfg = config_store.load().unwrap_or_default();
    let opts = build_options(&cfg.defaults, &args.tuning, None, preset_store, None)?;
    let raw_paths = collect_input_paths(&args.paths, args.filters.from_list.as_deref())?;
    let scan_opts =
        build_scan_options(&args.filters, cfg.recursive_by_default, cfg.max_input_bytes)?;
    let scan = scan_inputs(&raw_paths, &scan_opts)?;
    if scan.files.is_empty() {
        return Err(CaesiumCliError::new(
            ErrorCode::NoMatchingInputs,
            "No matching images found to preview",
        ));
    }

    if let Some(write_path) = &args.write_preview {
        if scan.files.len() > 1 {
            return Err(CaesiumCliError::new(
                ErrorCode::InvalidOption,
                "--write-preview can only be used with a single input image",
            ));
        }
        let raw = fs::read(&scan.files[0])?;
        let mem = compress_bytes(&raw, &opts)?;
        fs::write(write_path, &mem.bytes)?;
    }

    let mut previews = Vec::with_capacity(scan.files.len());
    for f in &scan.files {
        previews.push(preview_file(f, &opts)?);
    }

    match style {
        OutputStyle::Json => {
            print_json(&JsonEnvelope::ok(
                "preview",
                json!({ "items": previews }),
                opts.warnings(),
            ));
        }
        OutputStyle::Ndjson => {
            for p in &previews {
                print_ndjson_line(&json!({ "event": "preview", "item": p }));
            }
        }
        OutputStyle::Human => println!("{}", format_human_previews(&previews)),
        OutputStyle::Quiet => {}
    }

    Ok(exit_codes::SUCCESS)
}

fn handle_batch(args: BatchArgs, style: OutputStyle, preset_store: &PresetStore) -> CliResult<i32> {
    let raw = if args.spec == "-" {
        let mut s = String::new();
        io::stdin().read_to_string(&mut s)?;
        s
    } else {
        fs::read_to_string(&args.spec).map_err(|e| {
            CaesiumCliError::new(
                ErrorCode::InvalidInput,
                format!("Cannot read batch spec '{}': {e}", args.spec),
            )
        })?
    };

    let mut spec: BatchJobSpec = serde_json::from_str(&raw)?;
    if let Some(t) = args.threads {
        spec.threads = Some(t);
    }
    if args.dry_run {
        spec.dry_run = true;
    }

    let result = execute_batch_spec(&spec, preset_store)?;
    let failed = result["total_failed"].as_u64().unwrap_or(0);

    match style {
        OutputStyle::Json => print_json(&JsonEnvelope::ok("batch", &result, vec![])),
        OutputStyle::Ndjson => {
            print_ndjson_line(&json!({ "event": "batch_complete", "result": result }))
        }
        OutputStyle::Human => print_json(&result),
        OutputStyle::Quiet => {}
    }

    if failed == 0 {
        Ok(exit_codes::SUCCESS)
    } else {
        Ok(exit_codes::PARTIAL_FAILURE)
    }
}

fn handle_retry(args: RetryArgs, style: OutputStyle) -> CliResult<i32> {
    let report = retry_from_receipt(&args.receipt, args.threads)?;
    match style {
        OutputStyle::Json => {
            print_json(&JsonEnvelope::ok("retry", &report, report.warnings.clone()))
        }
        OutputStyle::Ndjson => {
            for item in &report.items {
                print_ndjson_line(&json!({ "event": "item", "item": item }));
            }
            print_ndjson_line(&json!({ "event": "complete", "summary": report.summary }));
        }
        OutputStyle::Human => println!("{}", format_human_report(&report)),
        OutputStyle::Quiet => {}
    }
    if report.summary.failed == 0 {
        Ok(exit_codes::SUCCESS)
    } else {
        Ok(exit_codes::PARTIAL_FAILURE)
    }
}

fn handle_preset(
    action: PresetCommands,
    style: OutputStyle,
    config_store: &ConfigStore,
    preset_store: &PresetStore,
) -> CliResult<i32> {
    match action {
        PresetCommands::List => {
            let list = preset_store.list()?;
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok("preset list", &list, vec![])),
                OutputStyle::Ndjson => {
                    for p in &list {
                        print_ndjson_line(&json!({ "preset": p }));
                    }
                }
                OutputStyle::Human => println!("{}", format_human_presets(&list)),
                OutputStyle::Quiet => {}
            }
        }
        PresetCommands::Show { name } | PresetCommands::Export { name } => {
            let entry = preset_store.get(&name)?;
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok("preset show", &entry, vec![])),
                _ => print_json(&entry),
            }
        }
        PresetCommands::Save {
            name,
            description,
            tuning,
        } => {
            let cfg = config_store.load().unwrap_or_default();
            let opts = build_options(&cfg.defaults, &tuning, None, preset_store, None)?;
            let saved = preset_store.save(&name, description, &opts)?;
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok("preset save", &saved, vec![])),
                OutputStyle::Human => println!("Saved preset '{}'", saved.name),
                _ => {}
            }
        }
        PresetCommands::Delete { name } => {
            preset_store.delete(&name)?;
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok(
                    "preset delete",
                    json!({ "deleted": name }),
                    vec![],
                )),
                OutputStyle::Human => println!("Deleted preset '{name}'"),
                _ => {}
            }
        }
        PresetCommands::Import { file, name } => {
            let raw = fs::read_to_string(&file)?;
            let val: Value = serde_json::from_str(&raw)?;
            let (preset_name, desc, opts) = if val.get("options").is_some() {
                let pname = name
                    .or_else(|| val.get("name").and_then(Value::as_str).map(String::from))
                    .ok_or_else(|| {
                        CaesiumCliError::new(
                            ErrorCode::InvalidOption,
                            "Imported preset JSON missing 'name' (pass --name)",
                        )
                    })?;
                let desc = val
                    .get("description")
                    .and_then(Value::as_str)
                    .map(String::from);
                let opts: CompressionOptions = serde_json::from_value(val["options"].clone())?;
                (pname, desc, opts)
            } else {
                let pname = name.ok_or_else(|| {
                    CaesiumCliError::new(
                        ErrorCode::InvalidOption,
                        "Pass --name when importing raw CompressionOptions JSON",
                    )
                })?;
                let opts: CompressionOptions = serde_json::from_value(val)?;
                (pname, None, opts)
            };
            let entry = preset_store.save(&preset_name, desc, &opts)?;
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok("preset import", &entry, vec![])),
                OutputStyle::Human => println!("Imported preset '{}'", entry.name),
                _ => {}
            }
        }
    }
    Ok(exit_codes::SUCCESS)
}

fn handle_config(
    action: ConfigCommands,
    style: OutputStyle,
    config_store: &ConfigStore,
) -> CliResult<i32> {
    match action {
        ConfigCommands::Show => {
            let cfg = config_store.load()?;
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok("config show", &cfg, vec![])),
                _ => print_json(&cfg),
            }
        }
        ConfigCommands::Path => {
            let p = config_store.config_path();
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok(
                    "config path",
                    json!({ "path": p }),
                    vec![],
                )),
                OutputStyle::Human => println!("{}", p.display()),
                _ => {}
            }
        }
        ConfigCommands::Reset => {
            let cfg = config_store.reset()?;
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok("config reset", &cfg, vec![])),
                OutputStyle::Human => {
                    println!("Reset config at {}", config_store.config_path().display())
                }
                _ => {}
            }
        }
        ConfigCommands::ImportGui { path } => {
            let ini_path = path
                .or_else(ConfigStore::default_gui_ini_path)
                .ok_or_else(|| {
                    CaesiumCliError::new(
                        ErrorCode::ConfigError,
                        "Could not determine Caesium GUI config directory",
                    )
                })?;
            let imported = ConfigStore::import_gui_ini(&ini_path)?;
            let mut cfg = config_store.load().unwrap_or_default();
            cfg.defaults = imported;
            config_store.save(&cfg)?;
            match style {
                OutputStyle::Json => print_json(&JsonEnvelope::ok(
                    "config import-gui",
                    json!({ "source": ini_path, "config": cfg }),
                    vec![],
                )),
                OutputStyle::Human => {
                    println!("Imported GUI settings from {}", ini_path.display());
                }
                _ => {}
            }
        }
    }
    Ok(exit_codes::SUCCESS)
}

fn handle_doctor(style: OutputStyle, config_store: &ConfigStore) -> CliResult<i32> {
    // Run live self-check of libcaesium codecs in memory
    let mut img = image::RgbImage::new(16, 16);
    for (x, y, p) in img.enumerate_pixels_mut() {
        *p = image::Rgb([(x * 16) as u8, (y * 16) as u8, 128]);
    }
    let dyn_img = image::DynamicImage::ImageRgb8(img);

    let mut png_buf = Vec::new();
    let _ = dyn_img.write_to(&mut io::Cursor::new(&mut png_buf), image::ImageFormat::Png);
    let mut jpg_buf = Vec::new();
    let _ = dyn_img.write_to(&mut io::Cursor::new(&mut jpg_buf), image::ImageFormat::Jpeg);

    let opts = CompressionOptions::default();
    let png_ok = compress_bytes(&png_buf, &opts).is_ok();
    let jpg_ok = compress_bytes(&jpg_buf, &opts).is_ok();

    let mut webp_opts = opts.clone();
    webp_opts.format = OutputFormat::WebP;
    let webp_ok = compress_bytes(&png_buf, &webp_opts).is_ok();

    let all_ok = png_ok && jpg_ok && webp_ok;
    let cpu_threads = std::thread::available_parallelism()
        .map(|n| n.get())
        .unwrap_or(1);
    let gui_ini = ConfigStore::default_gui_ini_path();
    let gui_ini_found = gui_ini.as_ref().map(|p| p.exists()).unwrap_or(false);

    let payload = json!({
        "healthy": all_ok,
        "app": APP_NAME,
        "version": APP_VERSION,
        "engine": "libcaesium 0.17.4",
        "cpu_threads": cpu_threads,
        "config_path": config_store.config_path(),
        "gui_settings_detected": gui_ini_found,
        "gui_settings_path": gui_ini,
        "max_input_default_bytes": DEFAULT_MAX_INPUT_BYTES,
        "codec_checks": {
            "jpeg": jpg_ok,
            "png": png_ok,
            "webp_convert": webp_ok
        }
    });

    match style {
        OutputStyle::Json => print_json(&JsonEnvelope::ok("doctor", &payload, vec![])),
        OutputStyle::Ndjson => print_ndjson_line(&json!({ "event": "doctor", "result": payload })),
        OutputStyle::Human => {
            println!("Caesium CLI Doctor ({APP_NAME} v{APP_VERSION})");
            println!("  Engine:       libcaesium 0.17.4");
            println!("  CPU threads:  {cpu_threads}");
            println!("  Config path:  {}", config_store.config_path().display());
            println!(
                "  GUI settings: {}",
                if gui_ini_found {
                    "detected"
                } else {
                    "not found"
                }
            );
            println!("  JPEG codec:   {}", if jpg_ok { "OK" } else { "FAIL" });
            println!("  PNG codec:    {}", if png_ok { "OK" } else { "FAIL" });
            println!("  WebP convert: {}", if webp_ok { "OK" } else { "FAIL" });
        }
        OutputStyle::Quiet => {}
    }

    Ok(if all_ok {
        exit_codes::SUCCESS
    } else {
        exit_codes::RUNTIME_ERROR
    })
}
