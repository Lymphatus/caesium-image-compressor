use crate::engine::ItemStatus;
use crate::options::{format_human_size, CompressionMode};
use crate::tui::state::{ActivePane, ActiveTab, ModalKind, TuiApp};
use crate::APP_VERSION;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::{
    Block, Borders, Cell, Clear, Gauge, List, ListItem, Paragraph, Row, Table, TableState, Tabs,
    Wrap,
};
use ratatui::Frame;

const ACCENT: Color = Color::Rgb(168, 85, 247); // Caesium dark theme purple
const SUCCESS: Color = Color::Rgb(34, 197, 94);
const WARN: Color = Color::Rgb(245, 158, 11);
const ERR: Color = Color::Rgb(239, 68, 68);
const MUTED: Color = Color::DarkGray;

pub fn render(frame: &mut Frame, app: &mut TuiApp) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3), // Header
            Constraint::Min(12),   // Main (Queue + Settings)
            Constraint::Length(6), // Inspector / Preview
            Constraint::Length(3), // Progress / Status
            Constraint::Length(1), // Keybindings footer
        ])
        .split(frame.area());

    render_header(frame, app, chunks[0]);
    render_main(frame, app, chunks[1]);
    render_inspector(frame, app, chunks[2]);
    render_status_bar(frame, app, chunks[3]);
    render_footer(frame, app, chunks[4]);

    if let Some(modal) = &app.modal {
        render_modal(frame, modal);
    }
}

fn render_header(frame: &mut Frame, app: &TuiApp, area: Rect) {
    let preset_tag = app
        .active_preset_name
        .as_deref()
        .map(|p| format!(" | Preset: {p}"))
        .unwrap_or_default();
    let title = format!(
        " Caesium Image Compressor TUI v{APP_VERSION} — {} image(s) ({}){preset_tag} ",
        app.queue.len(),
        format_human_size(app.total_original_bytes() as i64)
    );
    let p = Paragraph::new(Line::from(vec![Span::styled(
        title,
        Style::default().fg(ACCENT).add_modifier(Modifier::BOLD),
    )]))
    .block(
        Block::default()
            .borders(Borders::ALL)
            .border_style(Style::default().fg(ACCENT)),
    );
    frame.render_widget(p, area);
}

fn render_main(frame: &mut Frame, app: &TuiApp, area: Rect) {
    let cols = Layout::default()
        .direction(Direction::Horizontal)
        .constraints([Constraint::Percentage(58), Constraint::Percentage(42)])
        .split(area);

    render_queue(frame, app, cols[0]);
    render_settings(frame, app, cols[1]);
}

fn render_queue(frame: &mut Frame, app: &TuiApp, area: Rect) {
    let border_style = if app.active_pane == ActivePane::Queue {
        Style::default().fg(ACCENT)
    } else {
        Style::default().fg(MUTED)
    };

    let header = Row::new(["Name", "Size", "Resolution", "Saved", "Status"])
        .style(Style::default().fg(ACCENT).add_modifier(Modifier::BOLD));

    let rows: Vec<Row> = app
        .queue
        .iter()
        .map(|item| {
            let name = item
                .path
                .file_name()
                .map(|s| s.to_string_lossy().to_string())
                .unwrap_or_else(|| item.path.to_string_lossy().to_string());
            let orig_size = item.info.as_ref().map(|i| i.size_bytes).unwrap_or(0);
            let size_cell = match &item.result {
                Some(r) if r.status == ItemStatus::Compressed => format!(
                    "{} -> {}",
                    format_human_size(r.original_size as i64),
                    format_human_size(r.compressed_size as i64)
                ),
                _ => format_human_size(orig_size as i64),
            };
            let res_cell = match (&item.result, &item.info) {
                (Some(r), _) if r.compressed_dimensions.0 > 0 => {
                    format!(
                        "{}x{}",
                        r.compressed_dimensions.0, r.compressed_dimensions.1
                    )
                }
                (_, Some(i)) => format!("{}x{}", i.effective_width, i.effective_height),
                _ => "-".to_string(),
            };
            let (saved_cell, status_cell, status_color) = match &item.result {
                Some(r) => {
                    let s = format!("{:.1}%", r.saved_percent);
                    match r.status {
                        ItemStatus::Compressed => (s, "Compressed".to_string(), SUCCESS),
                        ItemStatus::Skipped => ("0.0%".to_string(), "Skipped".to_string(), WARN),
                        ItemStatus::Warning => (s, "Warning".to_string(), WARN),
                        ItemStatus::Error => ("-".to_string(), "Error".to_string(), ERR),
                        ItemStatus::Canceled => ("-".to_string(), "Canceled".to_string(), MUTED),
                    }
                }
                None => ("-".to_string(), "Ready".to_string(), Color::Gray),
            };
            Row::new([
                Cell::from(name),
                Cell::from(size_cell),
                Cell::from(res_cell),
                Cell::from(saved_cell),
                Cell::from(status_cell).style(Style::default().fg(status_color)),
            ])
        })
        .collect();

    let widths = [
        Constraint::Percentage(34),
        Constraint::Percentage(24),
        Constraint::Percentage(15),
        Constraint::Percentage(11),
        Constraint::Percentage(16),
    ];

    let title = format!(
        " Queue [Sort: {} {}] ",
        app.sort_column.label(),
        if app.sort_ascending { "▲" } else { "▼" }
    );
    let table = Table::new(rows, widths)
        .header(header)
        .block(
            Block::default()
                .title(title)
                .borders(Borders::ALL)
                .border_style(border_style),
        )
        .row_highlight_style(
            Style::default()
                .bg(Color::Rgb(55, 35, 85))
                .add_modifier(Modifier::BOLD),
        );

    let mut state = TableState::default();
    if !app.queue.is_empty() {
        state.select(Some(app.selected_queue));
    }
    frame.render_stateful_widget(table, area, &mut state);
}

fn render_settings(frame: &mut Frame, app: &TuiApp, area: Rect) {
    let border_style = if app.active_pane == ActivePane::Settings {
        Style::default().fg(ACCENT)
    } else {
        Style::default().fg(MUTED)
    };

    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Length(3), Constraint::Min(6)])
        .split(area);

    let tab_index = match app.active_tab {
        ActiveTab::Compression => 0,
        ActiveTab::Resize => 1,
        ActiveTab::Output => 2,
    };
    let tabs = Tabs::new(vec!["[1] Compression", "[2] Resize", "[3] Output"])
        .select(tab_index)
        .block(
            Block::default()
                .title(" Settings ")
                .borders(Borders::ALL)
                .border_style(border_style),
        )
        .highlight_style(Style::default().fg(ACCENT).add_modifier(Modifier::BOLD));
    frame.render_widget(tabs, chunks[0]);

    let o = &app.options;
    let items_data: Vec<(String, String)> = match app.active_tab {
        ActiveTab::Compression => vec![
            (
                "Mode".into(),
                match o.mode {
                    CompressionMode::Quality => "Quality".into(),
                    CompressionMode::Size => {
                        format!(
                            "Size ({}{})",
                            o.max_output_size.value,
                            o.max_output_size.unit.label()
                        )
                    }
                },
            ),
            ("JPEG Quality".into(), format!("{}%", o.jpeg_quality)),
            (
                "JPEG Chroma".into(),
                o.jpeg_chroma_subsampling.label().into(),
            ),
            ("JPEG Progressive".into(), yn(o.jpeg_progressive)),
            ("PNG Quality".into(), format!("{}%", o.png_quality)),
            (
                "PNG Opt Level".into(),
                format!("{} (0-6)", o.png_optimization_level),
            ),
            ("PNG Zopfli".into(), yn(o.png_force_zopfli)),
            ("WebP Quality".into(), format!("{}%", o.webp_quality)),
            ("GIF Quality".into(), format!("{}%", o.gif_quality)),
            ("TIFF Method".into(), format!("{:?}", o.tiff_method)),
            ("TIFF Deflate".into(), format!("{:?}", o.tiff_deflate_level)),
            ("Lossless".into(), yn(o.lossless)),
            ("Keep Metadata".into(), yn(o.keep_metadata)),
        ],
        ActiveTab::Resize => vec![
            ("Resize Mode".into(), o.resize.mode.label().into()),
            ("Width".into(), format!("{}", o.resize.width)),
            ("Height".into(), format!("{}", o.resize.height)),
            ("Edge Size".into(), format!("{} px", o.resize.edge)),
            ("Do Not Enlarge".into(), yn(o.resize.do_not_enlarge)),
            ("Keep Aspect Ratio".into(), yn(o.resize.keep_aspect_ratio)),
        ],
        ActiveTab::Output => vec![
            (
                "Output Folder [Enter]".into(),
                app.output_folder.to_string_lossy().to_string(),
            ),
            ("Same Folder as Input".into(), yn(o.same_folder_as_input)),
            ("Keep Structure".into(), yn(o.keep_structure)),
            ("Skip if Bigger".into(), yn(o.skip_if_bigger)),
            ("Move Original".into(), format!("{:?}", o.move_original)),
            ("Format".into(), o.format.canonical_name().to_uppercase()),
            ("Keep Modified Date".into(), yn(o.keep_dates.modified)),
            ("Keep Accessed Date".into(), yn(o.keep_dates.accessed)),
            (
                "Suffix [Enter]".into(),
                if o.suffix.is_empty() {
                    "(none)".into()
                } else {
                    o.suffix.clone()
                },
            ),
            (
                "Max Size Unit".into(),
                o.max_output_size.unit.label().into(),
            ),
        ],
    };

    let list_items: Vec<ListItem> = items_data
        .into_iter()
        .enumerate()
        .map(|(idx, (k, v))| {
            let selected = app.active_pane == ActivePane::Settings && idx == app.selected_setting;
            let prefix = if selected { "▸ " } else { "  " };
            let style = if selected {
                Style::default()
                    .fg(Color::White)
                    .bg(Color::Rgb(55, 35, 85))
                    .add_modifier(Modifier::BOLD)
            } else {
                Style::default()
            };
            ListItem::new(Line::from(vec![
                Span::styled(format!("{prefix}{k:<22}"), style),
                Span::styled(
                    v,
                    style.fg(if selected { Color::White } else { Color::Cyan }),
                ),
            ]))
        })
        .collect();

    let warnings = app.options.warnings();
    let title = if warnings.is_empty() {
        " ←/→ adjust · Space/Enter toggle/edit ".to_string()
    } else {
        format!(" ⚠ {} ", warnings[0])
    };
    let list = List::new(list_items).block(
        Block::default()
            .title(title)
            .borders(Borders::ALL)
            .border_style(if warnings.is_empty() {
                border_style
            } else {
                Style::default().fg(WARN)
            }),
    );
    frame.render_widget(list, chunks[1]);
}

fn render_inspector(frame: &mut Frame, app: &TuiApp, area: Rect) {
    let cols = Layout::default()
        .direction(Direction::Horizontal)
        .constraints([Constraint::Percentage(50), Constraint::Percentage(50)])
        .split(area);

    let orig_text = match app.queue.get(app.selected_queue) {
        Some(item) => match &item.info {
            Some(info) => format!(
                "File: {}\nFormat: {} ({}) | {}x{} (orient={}, {:.2} MP)\nSize: {} | EXIF: {} | ICC: {}",
                info.file_name,
                info.format.to_uppercase(),
                info.mime,
                info.effective_width,
                info.effective_height,
                info.exif_orientation,
                info.megapixels,
                info.human_size,
                yn(info.has_exif),
                yn(info.has_icc_profile),
            ),
            None => format!("File: {}", item.path.display()),
        },
        None => "No image selected. Press 'a' to add files or folders.".into(),
    };

    let preview_text = if let Some(prev) = &app.selected_preview {
        format!(
            "Format: {} -> {} | {}x{} -> {}x{}\nSize: {} -> {} (saved {}, {:.1}%)\nElapsed: {} ms{}",
            prev.input_format.to_uppercase(),
            prev.output_format.to_uppercase(),
            prev.original_dimensions.0,
            prev.original_dimensions.1,
            prev.compressed_dimensions.0,
            prev.compressed_dimensions.1,
            format_human_size(prev.original_size as i64),
            format_human_size(prev.compressed_size as i64),
            format_human_size(prev.saved_bytes),
            prev.saved_percent,
            prev.elapsed_ms,
            if prev.would_skip_if_bigger { " [would skip: bigger]" } else { "" }
        )
    } else if let Some(res) = app
        .queue
        .get(app.selected_queue)
        .and_then(|q| q.result.as_ref())
    {
        format!(
            "Status: {:?} | {} -> {}\nSaved: {} ({:.1}%) | {}x{}\n{}",
            res.status,
            format_human_size(res.original_size as i64),
            format_human_size(res.compressed_size as i64),
            format_human_size(res.saved_bytes),
            res.saved_percent,
            res.compressed_dimensions.0,
            res.compressed_dimensions.1,
            res.message.as_deref().unwrap_or("")
        )
    } else {
        "Press 'p' for live in-memory compression preview before writing files.".into()
    };

    frame.render_widget(
        Paragraph::new(orig_text)
            .wrap(Wrap { trim: true })
            .block(Block::default().title(" Original ").borders(Borders::ALL)),
        cols[0],
    );
    frame.render_widget(
        Paragraph::new(preview_text)
            .wrap(Wrap { trim: true })
            .block(
                Block::default()
                    .title(" Preview / Result ")
                    .borders(Borders::ALL),
            ),
        cols[1],
    );
}

fn render_status_bar(frame: &mut Frame, app: &TuiApp, area: Rect) {
    if app.compressing {
        let ratio = if app.progress_total > 0 {
            (app.progress_done as f64 / app.progress_total as f64).clamp(0.0, 1.0)
        } else {
            0.0
        };
        let label = format!(
            "Compressing ({}/{}) — press Esc or x to cancel",
            app.progress_done, app.progress_total
        );
        let gauge = Gauge::default()
            .block(Block::default().title(" Progress ").borders(Borders::ALL))
            .gauge_style(Style::default().fg(ACCENT))
            .ratio(ratio)
            .label(label);
        frame.render_widget(gauge, area);
    } else {
        let p = Paragraph::new(app.status_message.as_str())
            .block(Block::default().title(" Status ").borders(Borders::ALL));
        frame.render_widget(p, area);
    }
}

fn render_footer(frame: &mut Frame, _app: &TuiApp, area: Rect) {
    let hints = " [a] Add  [d] Remove  [x] Clear  [s] Sort  [p] Preview  [P] Presets  [c] Compress  [R] Retry Failed  [Tab] Pane  [1/2/3] Tabs  [?] Help  [q] Quit ";
    let p = Paragraph::new(hints).style(Style::default().fg(Color::Black).bg(ACCENT));
    frame.render_widget(p, area);
}

fn render_modal(frame: &mut Frame, modal: &ModalKind) {
    let area = centered_rect(68, 60, frame.area());
    frame.render_widget(Clear, area);

    match modal {
        ModalKind::Help => {
            let text = "\
Caesium TUI Keyboard Shortcuts

Navigation
  Tab             Switch between Queue and Settings panes
  j / k or ↑ / ↓  Move selection
  1 / 2 / 3       Switch settings tab (Compression / Resize / Output)
  h / l or ← / →  Adjust selected setting value
  Space / Enter   Toggle boolean or edit text field

Queue & Actions
  a               Add file or directory (with recursive + regex filters)
  d / Delete      Remove selected image
  x               Clear queue (or cancel while compressing)
  s / S           Cycle sort column / toggle ascending-descending
  p               Run live in-memory preview on selected image
  P               Open Preset Picker (web, lossless, thumbnail, social, ...)
  c               Start parallel compression
  R               Retry only failed images
  ?               Toggle this help screen
  q / Ctrl+C      Quit";
            let p = Paragraph::new(text).wrap(Wrap { trim: false }).block(
                Block::default()
                    .title(" Help (Esc/Enter to close) ")
                    .borders(Borders::ALL)
                    .border_style(Style::default().fg(ACCENT)),
            );
            frame.render_widget(p, area);
        }
        ModalKind::ConfirmOverwrite { .. } => {
            let text = "You are about to overwrite your original images in-place.\nThis action cannot be undone.\n\nPress [y] or [Enter] to overwrite, or [n] / [Esc] to cancel.";
            let p = Paragraph::new(text).wrap(Wrap { trim: true }).block(
                Block::default()
                    .title(" ⚠ Confirm Overwrite ")
                    .borders(Borders::ALL)
                    .border_style(Style::default().fg(WARN)),
            );
            frame.render_widget(p, area);
        }
        ModalKind::Summary(s) => {
            let text = format!(
                "Compression finished!\n\nTotal files:      {}\nCompressed:       {}\nSkipped:          {}\nWarnings:         {}\nFailed:           {}\n\nOriginal size:    {}\nCompressed size:  {}\nSaved:            {} ({:.2}%)\nElapsed:          {} ms\n\nPress Enter or Esc to close.",
                s.total,
                s.succeeded,
                s.skipped,
                s.warnings,
                s.failed,
                format_human_size(s.total_original_bytes as i64),
                format_human_size(s.total_compressed_bytes as i64),
                format_human_size(s.saved_bytes),
                s.saved_percent,
                s.elapsed_ms
            );
            let p = Paragraph::new(text).block(
                Block::default()
                    .title(" Compression Summary ")
                    .borders(Borders::ALL)
                    .border_style(Style::default().fg(SUCCESS)),
            );
            frame.render_widget(p, area);
        }
        ModalKind::PresetPicker { presets, selected } => {
            let items: Vec<ListItem> = presets
                .iter()
                .enumerate()
                .map(|(i, p)| {
                    let style = if i == *selected {
                        Style::default().bg(Color::Rgb(55, 35, 85)).fg(Color::White)
                    } else {
                        Style::default()
                    };
                    ListItem::new(format!(" {:<14} — {}", p.name, p.description)).style(style)
                })
                .collect();
            let list = List::new(items).block(
                Block::default()
                    .title(" Select Preset (Enter to apply, Esc to cancel) ")
                    .borders(Borders::ALL)
                    .border_style(Style::default().fg(ACCENT)),
            );
            frame.render_widget(list, area);
        }
        ModalKind::EditText { title, buffer, .. } => {
            let text = format!("{buffer}_\n\nEnter to save · Esc to cancel");
            let p = Paragraph::new(text).block(
                Block::default()
                    .title(format!(" Edit {title} "))
                    .borders(Borders::ALL)
                    .border_style(Style::default().fg(ACCENT)),
            );
            frame.render_widget(p, area);
        }
        ModalKind::AddPath {
            path,
            recursive,
            regex,
            focused_field,
        } => {
            let f0 = if *focused_field == 0 { ">" } else { " " };
            let f1 = if *focused_field == 1 { ">" } else { " " };
            let f2 = if *focused_field == 2 { ">" } else { " " };
            let text = format!(
                "{f0} Path (file or directory): {path}\n{f1} Include subfolders [Space]: {}\n{f2} Filename regex (optional): {regex}\n\nTab to switch field · Enter to import · Esc to cancel",
                yn(*recursive)
            );
            let p = Paragraph::new(text).block(
                Block::default()
                    .title(" Add Files / Advanced Import ")
                    .borders(Borders::ALL)
                    .border_style(Style::default().fg(ACCENT)),
            );
            frame.render_widget(p, area);
        }
    }
}

fn centered_rect(percent_x: u16, percent_y: u16, r: Rect) -> Rect {
    let vertical = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Percentage((100 - percent_y) / 2),
            Constraint::Percentage(percent_y),
            Constraint::Percentage((100 - percent_y) / 2),
        ])
        .split(r);
    Layout::default()
        .direction(Direction::Horizontal)
        .constraints([
            Constraint::Percentage((100 - percent_x) / 2),
            Constraint::Percentage(percent_x),
            Constraint::Percentage((100 - percent_x) / 2),
        ])
        .split(vertical[1])[1]
}

fn yn(v: bool) -> String {
    if v {
        "Yes".into()
    } else {
        "No".into()
    }
}
