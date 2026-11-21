mod common;

use caesium_cli::options::CompressionOptions;
use caesium_cli::tui::state::{ActiveTab, ModalKind, TuiApp};
use caesium_cli::tui::ui::render;
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};
use image::ImageFormat;
use ratatui::backend::TestBackend;
use ratatui::Terminal;
use tempfile::TempDir;

fn press(code: KeyCode) -> KeyEvent {
    KeyEvent::new(code, KeyModifiers::NONE)
}

#[test]
fn tui_renders_queue_tabs_and_handles_navigation_and_preview() {
    let dir = TempDir::new().unwrap();
    let img1 = common::save_fixture(dir.path(), "sunset.jpg", 160, 120, ImageFormat::Jpeg);
    let img2 = common::save_fixture(dir.path(), "logo.png", 100, 100, ImageFormat::Png);

    let mut app = TuiApp::new(vec![img1, img2], CompressionOptions::default(), None);
    assert_eq!(app.queue.len(), 2);
    assert_eq!(app.active_tab, ActiveTab::Compression);

    // Render onto headless TestBackend
    let backend = TestBackend::new(120, 36);
    let mut terminal = Terminal::new(backend).unwrap();
    terminal.draw(|frame| render(frame, &mut app)).unwrap();
    let buf_text: String = terminal
        .backend()
        .buffer()
        .content()
        .iter()
        .map(|c| c.symbol())
        .collect();
    assert!(buf_text.contains("Caesium"));
    assert!(buf_text.contains("sunset.jpg"));
    assert!(buf_text.contains("Compression"));
    assert!(buf_text.contains("Resize"));
    assert!(buf_text.contains("Output"));

    // Switch settings tabs with 2 and 3
    app.handle_key(press(KeyCode::Char('2')));
    assert_eq!(app.active_tab, ActiveTab::Resize);
    terminal.draw(|frame| render(frame, &mut app)).unwrap();

    app.handle_key(press(KeyCode::Char('3')));
    assert_eq!(app.active_tab, ActiveTab::Output);
    terminal.draw(|frame| render(frame, &mut app)).unwrap();

    // Open and close help overlay
    app.handle_key(press(KeyCode::Char('?')));
    assert!(matches!(app.modal, Some(ModalKind::Help)));
    terminal.draw(|frame| render(frame, &mut app)).unwrap();
    app.handle_key(press(KeyCode::Esc));
    assert!(app.modal.is_none());

    // Live preview on selected item
    app.handle_key(press(KeyCode::Char('p')));
    assert!(app.selected_preview.is_some());

    // Quit cleanly
    app.handle_key(press(KeyCode::Char('q')));
    assert!(app.should_quit);
}
