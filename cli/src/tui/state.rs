use crate::config::{PresetEntry, PresetStore};
use crate::engine::{
    dry_run_batch, preview_file, run_batch_with_progress, BatchReport, BatchSummary, ItemResult,
    ItemStatus, OutputTarget, PreviewResult,
};
use crate::inspect::{inspect_path, ImageInfo};
use crate::options::{
    ChromaSubsamplingOption, CompressionMode, CompressionOptions, ImportFilters, MaxOutputSizeUnit,
    MoveOriginalAction, OutputFormat, ResizeMode, TiffCompressionOption, TiffDeflateLevelOption,
    DEFAULT_MAX_INPUT_BYTES,
};
use crate::scanner::{common_root_folder, scan_inputs, ScanOptions};
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};
use std::collections::HashSet;
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc::{self, Receiver};
use std::sync::Arc;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ActivePane {
    Queue,
    Settings,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ActiveTab {
    Compression,
    Resize,
    Output,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SortColumn {
    Name,
    Size,
    Resolution,
    Ratio,
    Status,
}

impl SortColumn {
    pub fn next(self) -> Self {
        match self {
            SortColumn::Name => SortColumn::Size,
            SortColumn::Size => SortColumn::Resolution,
            SortColumn::Resolution => SortColumn::Ratio,
            SortColumn::Ratio => SortColumn::Status,
            SortColumn::Status => SortColumn::Name,
        }
    }

    pub fn label(self) -> &'static str {
        match self {
            SortColumn::Name => "Name",
            SortColumn::Size => "Size",
            SortColumn::Resolution => "Resolution",
            SortColumn::Ratio => "Saved %",
            SortColumn::Status => "Status",
        }
    }
}

#[derive(Debug, Clone)]
pub struct QueueItem {
    pub path: PathBuf,
    pub info: Option<ImageInfo>,
    pub result: Option<ItemResult>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TextFieldKind {
    OutputFolder,
    Suffix,
}

#[derive(Debug, Clone)]
pub enum ModalKind {
    Help,
    AddPath {
        path: String,
        recursive: bool,
        regex: String,
        focused_field: usize,
    },
    PresetPicker {
        presets: Vec<PresetEntry>,
        selected: usize,
    },
    EditText {
        title: String,
        field: TextFieldKind,
        buffer: String,
    },
    ConfirmOverwrite {
        only_failed: bool,
    },
    Summary(BatchSummary),
}

pub enum WorkerEvent {
    ItemDone(ItemResult),
    Finished(Result<BatchReport, String>),
}

pub struct TuiApp {
    pub queue: Vec<QueueItem>,
    pub selected_queue: usize,
    pub active_pane: ActivePane,
    pub active_tab: ActiveTab,
    pub selected_setting: usize,
    pub sort_column: SortColumn,
    pub sort_ascending: bool,

    pub options: CompressionOptions,
    pub output_folder: PathBuf,
    pub active_preset_name: Option<String>,
    pub threads: usize,

    pub selected_preview: Option<PreviewResult>,
    pub status_message: String,
    pub modal: Option<ModalKind>,
    pub should_quit: bool,

    // Background compression worker state
    pub compressing: bool,
    pub progress_done: usize,
    pub progress_total: usize,
    worker_rx: Option<Receiver<WorkerEvent>>,
    cancel_flag: Option<Arc<AtomicBool>>,
    presets: PresetStore,
}

impl TuiApp {
    pub fn new(
        initial_paths: Vec<PathBuf>,
        initial_options: CompressionOptions,
        output_dir: Option<PathBuf>,
    ) -> Self {
        let default_out = output_dir.unwrap_or_else(|| {
            std::env::current_dir()
                .unwrap_or_else(|_| PathBuf::from("."))
                .join("compressed")
        });
        let threads = std::thread::available_parallelism()
            .map(|n| n.get())
            .unwrap_or(2);

        let mut app = Self {
            queue: Vec::new(),
            selected_queue: 0,
            active_pane: ActivePane::Queue,
            active_tab: ActiveTab::Compression,
            selected_setting: 0,
            sort_column: SortColumn::Name,
            sort_ascending: true,
            options: initial_options,
            output_folder: default_out,
            active_preset_name: None,
            threads,
            selected_preview: None,
            status_message:
                "Ready — press 'a' to add images, 'p' to preview, 'c' to compress, '?' for help"
                    .into(),
            modal: None,
            should_quit: false,
            compressing: false,
            progress_done: 0,
            progress_total: 0,
            worker_rx: None,
            cancel_flag: None,
            presets: PresetStore::default_store(),
        };

        if !initial_paths.is_empty() {
            app.add_paths(&initial_paths, true, ImportFilters::default());
        }

        app
    }

    pub fn add_paths(&mut self, paths: &[PathBuf], recursive: bool, filters: ImportFilters) {
        let scan = match scan_inputs(
            paths,
            &ScanOptions {
                recursive,
                filters,
                max_input_bytes: Some(DEFAULT_MAX_INPUT_BYTES),
            },
        ) {
            Ok(s) => s,
            Err(e) => {
                self.status_message = format!("Import error: {}", e.message);
                return;
            }
        };

        let existing: HashSet<PathBuf> = self.queue.iter().map(|q| q.path.clone()).collect();
        let mut added = 0usize;
        for f in scan.files {
            if !existing.contains(&f) {
                let info = inspect_path(&f).ok();
                self.queue.push(QueueItem {
                    path: f,
                    info,
                    result: None,
                });
                added += 1;
            }
        }
        self.apply_sort();
        self.status_message = format!(
            "Added {added} image(s) ({} skipped). Total in queue: {}",
            scan.skipped.len(),
            self.queue.len()
        );
    }

    pub fn total_original_bytes(&self) -> u64 {
        self.queue
            .iter()
            .map(|q| q.info.as_ref().map(|i| i.size_bytes).unwrap_or(0))
            .sum()
    }

    pub fn settings_count_for_tab(&self) -> usize {
        match self.active_tab {
            ActiveTab::Compression => 13,
            ActiveTab::Resize => 6,
            ActiveTab::Output => 10,
        }
    }

    pub fn handle_key(&mut self, key: KeyEvent) {
        // Ctrl+C always exits cleanly
        if key.modifiers.contains(KeyModifiers::CONTROL) && key.code == KeyCode::Char('c') {
            self.cancel_compression();
            self.should_quit = true;
            return;
        }

        // Modal key handling takes priority
        if self.modal.is_some() {
            self.handle_modal_key(key);
            return;
        }

        // While compressing, only allow cancel/quit/navigation
        if self.compressing {
            match key.code {
                KeyCode::Esc | KeyCode::Char('x') => {
                    self.cancel_compression();
                }
                KeyCode::Char('q') => {
                    self.cancel_compression();
                    self.should_quit = true;
                }
                _ => {}
            }
            return;
        }

        match key.code {
            KeyCode::Char('q') => self.should_quit = true,
            KeyCode::Char('?') => self.modal = Some(ModalKind::Help),
            KeyCode::Tab => {
                self.active_pane = match self.active_pane {
                    ActivePane::Queue => ActivePane::Settings,
                    ActivePane::Settings => ActivePane::Queue,
                };
            }
            KeyCode::Char('1') => {
                self.active_tab = ActiveTab::Compression;
                self.selected_setting = 0;
            }
            KeyCode::Char('2') => {
                self.active_tab = ActiveTab::Resize;
                self.selected_setting = 0;
            }
            KeyCode::Char('3') => {
                self.active_tab = ActiveTab::Output;
                self.selected_setting = 0;
            }
            KeyCode::Char('a') => {
                self.modal = Some(ModalKind::AddPath {
                    path: String::new(),
                    recursive: true,
                    regex: String::new(),
                    focused_field: 0,
                });
            }
            KeyCode::Char('d') | KeyCode::Delete => {
                if !self.queue.is_empty() && self.selected_queue < self.queue.len() {
                    self.queue.remove(self.selected_queue);
                    if self.selected_queue >= self.queue.len() && self.selected_queue > 0 {
                        self.selected_queue -= 1;
                    }
                    self.selected_preview = None;
                    self.status_message = format!("Removed item. Queue size: {}", self.queue.len());
                }
            }
            KeyCode::Char('x') => {
                self.queue.clear();
                self.selected_queue = 0;
                self.selected_preview = None;
                self.status_message = "Queue cleared.".into();
            }
            KeyCode::Char('s') => {
                self.sort_column = self.sort_column.next();
                self.apply_sort();
                self.status_message = format!("Sorted by {}", self.sort_column.label());
            }
            KeyCode::Char('S') => {
                self.sort_ascending = !self.sort_ascending;
                self.apply_sort();
            }
            KeyCode::Char('p') => {
                self.run_selected_preview();
            }
            KeyCode::Char('P') => {
                if let Ok(presets) = self.presets.list() {
                    self.modal = Some(ModalKind::PresetPicker {
                        presets,
                        selected: 0,
                    });
                }
            }
            KeyCode::Char('c') => {
                self.request_compression(false);
            }
            KeyCode::Char('R') => {
                self.request_compression(true);
            }
            KeyCode::Up | KeyCode::Char('k') => match self.active_pane {
                ActivePane::Queue => {
                    if self.selected_queue > 0 {
                        self.selected_queue -= 1;
                        self.selected_preview = None;
                    }
                }
                ActivePane::Settings => {
                    if self.selected_setting > 0 {
                        self.selected_setting -= 1;
                    }
                }
            },
            KeyCode::Down | KeyCode::Char('j') => match self.active_pane {
                ActivePane::Queue => {
                    if self.selected_queue + 1 < self.queue.len() {
                        self.selected_queue += 1;
                        self.selected_preview = None;
                    }
                }
                ActivePane::Settings => {
                    if self.selected_setting + 1 < self.settings_count_for_tab() {
                        self.selected_setting += 1;
                    }
                }
            },
            KeyCode::Left | KeyCode::Char('h') | KeyCode::Char('-') => {
                if self.active_pane == ActivePane::Settings {
                    self.adjust_setting(-1);
                }
            }
            KeyCode::Right | KeyCode::Char('l') | KeyCode::Char('+') | KeyCode::Char(' ') => {
                if self.active_pane == ActivePane::Settings {
                    self.adjust_setting(1);
                }
            }
            KeyCode::Enter => {
                if self.active_pane == ActivePane::Settings {
                    self.activate_setting();
                } else {
                    self.run_selected_preview();
                }
            }
            _ => {}
        }
    }

    fn handle_modal_key(&mut self, key: KeyEvent) {
        let Some(modal) = self.modal.take() else {
            return;
        };
        match modal {
            ModalKind::Help | ModalKind::Summary(_) => {
                if !matches!(
                    key.code,
                    KeyCode::Esc | KeyCode::Enter | KeyCode::Char('q') | KeyCode::Char('?')
                ) {
                    self.modal = Some(modal);
                }
            }
            ModalKind::ConfirmOverwrite { only_failed } => match key.code {
                KeyCode::Char('y') | KeyCode::Char('Y') | KeyCode::Enter => {
                    self.options.overwrite = true;
                    self.start_compression(only_failed);
                }
                KeyCode::Char('n') | KeyCode::Char('N') | KeyCode::Esc => {
                    self.status_message = "Compression canceled (originals untouched).".into();
                }
                _ => {
                    self.modal = Some(ModalKind::ConfirmOverwrite { only_failed });
                }
            },
            ModalKind::PresetPicker {
                presets,
                mut selected,
            } => match key.code {
                KeyCode::Esc => {}
                KeyCode::Up | KeyCode::Char('k') => {
                    selected = selected.saturating_sub(1);
                    self.modal = Some(ModalKind::PresetPicker { presets, selected });
                }
                KeyCode::Down | KeyCode::Char('j') => {
                    if selected + 1 < presets.len() {
                        selected += 1;
                    }
                    self.modal = Some(ModalKind::PresetPicker { presets, selected });
                }
                KeyCode::Enter => {
                    if let Some(entry) = presets.get(selected) {
                        self.options = entry.options.clone();
                        self.active_preset_name = Some(entry.name.clone());
                        self.selected_preview = None;
                        self.status_message = format!("Applied preset '{}'", entry.name);
                    }
                }
                _ => {
                    self.modal = Some(ModalKind::PresetPicker { presets, selected });
                }
            },
            ModalKind::EditText {
                title,
                field,
                mut buffer,
            } => match key.code {
                KeyCode::Esc => {}
                KeyCode::Enter => {
                    match field {
                        TextFieldKind::OutputFolder => {
                            if !buffer.trim().is_empty() {
                                self.output_folder = PathBuf::from(buffer.trim());
                            }
                        }
                        TextFieldKind::Suffix => {
                            self.options.suffix = buffer;
                        }
                    }
                    self.status_message = "Updated setting.".into();
                }
                KeyCode::Backspace => {
                    buffer.pop();
                    self.modal = Some(ModalKind::EditText {
                        title,
                        field,
                        buffer,
                    });
                }
                KeyCode::Char(c) => {
                    buffer.push(c);
                    self.modal = Some(ModalKind::EditText {
                        title,
                        field,
                        buffer,
                    });
                }
                _ => {
                    self.modal = Some(ModalKind::EditText {
                        title,
                        field,
                        buffer,
                    });
                }
            },
            ModalKind::AddPath {
                mut path,
                mut recursive,
                mut regex,
                mut focused_field,
            } => match key.code {
                KeyCode::Esc => {}
                KeyCode::Tab => {
                    focused_field = (focused_field + 1) % 3;
                    self.modal = Some(ModalKind::AddPath {
                        path,
                        recursive,
                        regex,
                        focused_field,
                    });
                }
                KeyCode::Enter => {
                    if !path.trim().is_empty() {
                        let filters = ImportFilters {
                            filename_regex: if regex.trim().is_empty() {
                                None
                            } else {
                                Some(regex.trim().to_string())
                            },
                            ..Default::default()
                        };
                        self.add_paths(&[PathBuf::from(path.trim())], recursive, filters);
                    }
                }
                KeyCode::Char(' ') if focused_field == 1 => {
                    recursive = !recursive;
                    self.modal = Some(ModalKind::AddPath {
                        path,
                        recursive,
                        regex,
                        focused_field,
                    });
                }
                KeyCode::Backspace => {
                    if focused_field == 0 {
                        path.pop();
                    } else if focused_field == 2 {
                        regex.pop();
                    }
                    self.modal = Some(ModalKind::AddPath {
                        path,
                        recursive,
                        regex,
                        focused_field,
                    });
                }
                KeyCode::Char(c) => {
                    if focused_field == 0 {
                        path.push(c);
                    } else if focused_field == 2 {
                        regex.push(c);
                    }
                    self.modal = Some(ModalKind::AddPath {
                        path,
                        recursive,
                        regex,
                        focused_field,
                    });
                }
                _ => {
                    self.modal = Some(ModalKind::AddPath {
                        path,
                        recursive,
                        regex,
                        focused_field,
                    });
                }
            },
        }
    }

    fn activate_setting(&mut self) {
        if self.active_tab == ActiveTab::Output {
            match self.selected_setting {
                0 => {
                    self.modal = Some(ModalKind::EditText {
                        title: "Output Folder".into(),
                        field: TextFieldKind::OutputFolder,
                        buffer: self.output_folder.to_string_lossy().to_string(),
                    });
                    return;
                }
                8 => {
                    self.modal = Some(ModalKind::EditText {
                        title: "Filename Suffix".into(),
                        field: TextFieldKind::Suffix,
                        buffer: self.options.suffix.clone(),
                    });
                    return;
                }
                _ => {}
            }
        }
        self.adjust_setting(1);
    }

    fn adjust_setting(&mut self, delta: i32) {
        self.active_preset_name = None;
        self.selected_preview = None;
        match self.active_tab {
            ActiveTab::Compression => match self.selected_setting {
                0 => {
                    self.options.mode = match self.options.mode {
                        CompressionMode::Quality => CompressionMode::Size,
                        CompressionMode::Size => CompressionMode::Quality,
                    };
                }
                1 => {
                    self.options.jpeg_quality =
                        (self.options.jpeg_quality as i32 + delta * 5).clamp(0, 100) as u32;
                }
                2 => {
                    let all = [
                        ChromaSubsamplingOption::Auto,
                        ChromaSubsamplingOption::Cs444,
                        ChromaSubsamplingOption::Cs422,
                        ChromaSubsamplingOption::Cs420,
                        ChromaSubsamplingOption::Cs411,
                    ];
                    let idx = all
                        .iter()
                        .position(|c| *c == self.options.jpeg_chroma_subsampling)
                        .unwrap_or(0) as i32;
                    let next = (idx + delta).rem_euclid(all.len() as i32) as usize;
                    self.options.jpeg_chroma_subsampling = all[next];
                }
                3 => self.options.jpeg_progressive = !self.options.jpeg_progressive,
                4 => {
                    self.options.png_quality =
                        (self.options.png_quality as i32 + delta * 5).clamp(0, 100) as u32;
                }
                5 => {
                    self.options.png_optimization_level =
                        (self.options.png_optimization_level as i32 + delta).clamp(0, 6) as u8;
                }
                6 => self.options.png_force_zopfli = !self.options.png_force_zopfli,
                7 => {
                    self.options.webp_quality =
                        (self.options.webp_quality as i32 + delta * 5).clamp(0, 100) as u32;
                }
                8 => {
                    self.options.gif_quality =
                        (self.options.gif_quality as i32 + delta * 5).clamp(0, 100) as u32;
                }
                9 => {
                    let all = [
                        TiffCompressionOption::Uncompressed,
                        TiffCompressionOption::Lzw,
                        TiffCompressionOption::Deflate,
                        TiffCompressionOption::Packbits,
                    ];
                    let idx = all
                        .iter()
                        .position(|c| *c == self.options.tiff_method)
                        .unwrap_or(1) as i32;
                    self.options.tiff_method =
                        all[(idx + delta).rem_euclid(all.len() as i32) as usize];
                }
                10 => {
                    let all = [
                        TiffDeflateLevelOption::Fast,
                        TiffDeflateLevelOption::Balanced,
                        TiffDeflateLevelOption::Best,
                    ];
                    let idx = all
                        .iter()
                        .position(|c| *c == self.options.tiff_deflate_level)
                        .unwrap_or(1) as i32;
                    self.options.tiff_deflate_level =
                        all[(idx + delta).rem_euclid(all.len() as i32) as usize];
                }
                11 => self.options.lossless = !self.options.lossless,
                12 => self.options.keep_metadata = !self.options.keep_metadata,
                _ => {}
            },
            ActiveTab::Resize => match self.selected_setting {
                0 => {
                    let all = [
                        ResizeMode::None,
                        ResizeMode::Dimensions,
                        ResizeMode::Percentage,
                        ResizeMode::ShortEdge,
                        ResizeMode::LongEdge,
                        ResizeMode::FixedWidth,
                        ResizeMode::FixedHeight,
                    ];
                    let idx = all
                        .iter()
                        .position(|m| *m == self.options.resize.mode)
                        .unwrap_or(0) as i32;
                    self.options.resize.mode =
                        all[(idx + delta).rem_euclid(all.len() as i32) as usize];
                }
                1 => {
                    let step = if self.options.resize.mode == ResizeMode::Percentage {
                        5
                    } else {
                        50
                    };
                    let max = if self.options.resize.mode == ResizeMode::Percentage {
                        100
                    } else {
                        99999
                    };
                    self.options.resize.width =
                        (self.options.resize.width as i32 + delta * step).clamp(1, max) as u32;
                }
                2 => {
                    let step = if self.options.resize.mode == ResizeMode::Percentage {
                        5
                    } else {
                        50
                    };
                    let max = if self.options.resize.mode == ResizeMode::Percentage {
                        100
                    } else {
                        99999
                    };
                    self.options.resize.height =
                        (self.options.resize.height as i32 + delta * step).clamp(1, max) as u32;
                }
                3 => {
                    self.options.resize.edge =
                        (self.options.resize.edge as i32 + delta * 50).clamp(1, 99999) as u32;
                }
                4 => self.options.resize.do_not_enlarge = !self.options.resize.do_not_enlarge,
                5 => self.options.resize.keep_aspect_ratio = !self.options.resize.keep_aspect_ratio,
                _ => {}
            },
            ActiveTab::Output => match self.selected_setting {
                1 => self.options.same_folder_as_input = !self.options.same_folder_as_input,
                2 => self.options.keep_structure = !self.options.keep_structure,
                3 => self.options.skip_if_bigger = !self.options.skip_if_bigger,
                4 => {
                    let all = [
                        MoveOriginalAction::None,
                        MoveOriginalAction::Trash,
                        MoveOriginalAction::Delete,
                    ];
                    let idx = all
                        .iter()
                        .position(|a| *a == self.options.move_original)
                        .unwrap_or(0) as i32;
                    self.options.move_original =
                        all[(idx + delta).rem_euclid(all.len() as i32) as usize];
                }
                5 => {
                    let all = [
                        OutputFormat::Same,
                        OutputFormat::Jpg,
                        OutputFormat::Png,
                        OutputFormat::WebP,
                        OutputFormat::Tiff,
                        OutputFormat::Gif,
                    ];
                    let idx = all
                        .iter()
                        .position(|f| *f == self.options.format)
                        .unwrap_or(0) as i32;
                    self.options.format = all[(idx + delta).rem_euclid(all.len() as i32) as usize];
                }
                6 => self.options.keep_dates.modified = !self.options.keep_dates.modified,
                7 => self.options.keep_dates.accessed = !self.options.keep_dates.accessed,
                9 => {
                    // Size mode target value adjustment shortcut when in size mode, or max_size unit cycle
                    let units = [
                        MaxOutputSizeUnit::Bytes,
                        MaxOutputSizeUnit::Kilobytes,
                        MaxOutputSizeUnit::Megabytes,
                        MaxOutputSizeUnit::Percentage,
                    ];
                    let idx = units
                        .iter()
                        .position(|u| *u == self.options.max_output_size.unit)
                        .unwrap_or(1) as i32;
                    self.options.max_output_size.unit =
                        units[(idx + delta).rem_euclid(units.len() as i32) as usize];
                }
                _ => {}
            },
        }
    }

    pub fn run_selected_preview(&mut self) {
        let Some(item) = self.queue.get(self.selected_queue) else {
            self.status_message = "No image selected for preview.".into();
            return;
        };
        match preview_file(&item.path, &self.options) {
            Ok(prev) => {
                self.status_message = format!(
                    "Preview: {} -> {} ({:.1}% saved) in {} ms",
                    crate::options::format_human_size(prev.original_size as i64),
                    crate::options::format_human_size(prev.compressed_size as i64),
                    prev.saved_percent,
                    prev.elapsed_ms
                );
                self.selected_preview = Some(prev);
            }
            Err(e) => {
                self.status_message = format!("Preview failed: {}", e.message);
            }
        }
    }

    fn current_output_target(&self) -> OutputTarget {
        if self.options.same_folder_as_input {
            OutputTarget::SameFolder
        } else {
            OutputTarget::Directory(self.output_folder.clone())
        }
    }

    pub fn request_compression(&mut self, only_failed: bool) {
        let inputs: Vec<PathBuf> = self
            .queue
            .iter()
            .filter(|q| {
                !only_failed
                    || matches!(
                        q.result.as_ref().map(|r| r.status),
                        Some(ItemStatus::Error | ItemStatus::Canceled)
                    )
            })
            .map(|q| q.path.clone())
            .collect();
        if inputs.is_empty() {
            self.status_message = if only_failed {
                "No failed items to retry.".into()
            } else {
                "Queue is empty — press 'a' to add files or folders.".into()
            };
            return;
        }

        let target = self.current_output_target();
        let root = common_root_folder(&inputs);
        if let Ok(plan) = dry_run_batch(&inputs, target, &self.options, Some(root)) {
            if plan.requires_overwrite && !self.options.overwrite {
                self.modal = Some(ModalKind::ConfirmOverwrite { only_failed });
                return;
            }
        }
        self.start_compression(only_failed);
    }

    fn start_compression(&mut self, only_failed: bool) {
        let inputs: Vec<PathBuf> = self
            .queue
            .iter()
            .filter(|q| {
                !only_failed
                    || matches!(
                        q.result.as_ref().map(|r| r.status),
                        Some(ItemStatus::Error | ItemStatus::Canceled)
                    )
            })
            .map(|q| q.path.clone())
            .collect();
        if inputs.is_empty() {
            return;
        }

        let target = self.current_output_target();
        let root = common_root_folder(&inputs);
        let opts = self.options.clone();
        let threads = self.threads;

        let (tx, rx) = mpsc::channel();
        let cancel = Arc::new(AtomicBool::new(false));
        let cancel_worker = Arc::clone(&cancel);

        self.compressing = true;
        self.progress_done = 0;
        self.progress_total = inputs.len();
        self.worker_rx = Some(rx);
        self.cancel_flag = Some(cancel);
        self.status_message = format!("Compressing {} image(s)...", inputs.len());

        std::thread::spawn(move || {
            let tx_item = tx.clone();
            let res = run_batch_with_progress(
                &inputs,
                target,
                &opts,
                threads,
                Some(root),
                Some(cancel_worker),
                move |item| {
                    let _ = tx_item.send(WorkerEvent::ItemDone(item.clone()));
                },
            )
            .map_err(|e| e.message);
            let _ = tx.send(WorkerEvent::Finished(res));
        });
    }

    pub fn cancel_compression(&mut self) {
        if let Some(flag) = &self.cancel_flag {
            flag.store(true, Ordering::Relaxed);
            self.status_message = "Canceling compression...".into();
        }
    }

    pub fn poll_worker(&mut self) {
        let Some(rx) = &self.worker_rx else {
            return;
        };
        let mut finished = false;
        while let Ok(ev) = rx.try_recv() {
            match ev {
                WorkerEvent::ItemDone(item_res) => {
                    self.progress_done += 1;
                    if let Some(entry) = self
                        .queue
                        .iter_mut()
                        .find(|q| q.path == item_res.input_path)
                    {
                        entry.result = Some(item_res);
                    }
                }
                WorkerEvent::Finished(Ok(report)) => {
                    self.compressing = false;
                    self.status_message = format!(
                        "Finished: {}/{} compressed, saved {} ({:.1}%)",
                        report.summary.succeeded,
                        report.summary.total,
                        crate::options::format_human_size(report.summary.saved_bytes),
                        report.summary.saved_percent
                    );
                    self.modal = Some(ModalKind::Summary(report.summary));
                    finished = true;
                }
                WorkerEvent::Finished(Err(msg)) => {
                    self.compressing = false;
                    self.status_message = format!("Compression error: {msg}");
                    finished = true;
                }
            }
        }
        if finished {
            self.worker_rx = None;
            self.cancel_flag = None;
        }
    }

    fn apply_sort(&mut self) {
        let asc = self.sort_ascending;
        match self.sort_column {
            SortColumn::Name => self.queue.sort_by(|a, b| {
                let ord = a.path.file_name().cmp(&b.path.file_name());
                if asc {
                    ord
                } else {
                    ord.reverse()
                }
            }),
            SortColumn::Size => self.queue.sort_by(|a, b| {
                let sa = a.info.as_ref().map(|i| i.size_bytes).unwrap_or(0);
                let sb = b.info.as_ref().map(|i| i.size_bytes).unwrap_or(0);
                let ord = sa.cmp(&sb);
                if asc {
                    ord
                } else {
                    ord.reverse()
                }
            }),
            SortColumn::Resolution => self.queue.sort_by(|a, b| {
                let pa = a.info.as_ref().map(|i| i.total_pixels).unwrap_or(0);
                let pb = b.info.as_ref().map(|i| i.total_pixels).unwrap_or(0);
                let ord = pa.cmp(&pb);
                if asc {
                    ord
                } else {
                    ord.reverse()
                }
            }),
            SortColumn::Ratio => self.queue.sort_by(|a, b| {
                let ra = a.result.as_ref().map(|r| r.saved_percent).unwrap_or(0.0);
                let rb = b.result.as_ref().map(|r| r.saved_percent).unwrap_or(0.0);
                let ord = ra.total_cmp(&rb);
                if asc {
                    ord
                } else {
                    ord.reverse()
                }
            }),
            SortColumn::Status => self.queue.sort_by(|a, b| {
                let sa = format!("{:?}", a.result.as_ref().map(|r| r.status));
                let sb = format!("{:?}", b.result.as_ref().map(|r| r.status));
                let ord = sa.cmp(&sb);
                if asc {
                    ord
                } else {
                    ord.reverse()
                }
            }),
        }
    }
}
