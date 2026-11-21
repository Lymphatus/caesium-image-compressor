use caesium_cli::config::{ConfigStore, PresetStore};
use caesium_cli::options::{
    ChromaSubsamplingOption, CompressionMode, CompressionOptions, MaxOutputSize, MaxOutputSizeUnit,
    OutputFormat, ResizeMode, ResizeOptions, TiffCompressionOption, TiffDeflateLevelOption,
};
use tempfile::TempDir;

#[test]
fn parses_max_output_size_strings() {
    assert_eq!(
        MaxOutputSize::parse("500").unwrap(),
        MaxOutputSize {
            value: 500,
            unit: MaxOutputSizeUnit::Bytes,
        }
    );
    assert_eq!(
        MaxOutputSize::parse("250KB").unwrap(),
        MaxOutputSize {
            value: 250,
            unit: MaxOutputSizeUnit::Kilobytes,
        }
    );
    assert_eq!(
        MaxOutputSize::parse("2mb").unwrap(),
        MaxOutputSize {
            value: 2,
            unit: MaxOutputSizeUnit::Megabytes,
        }
    );
    assert_eq!(
        MaxOutputSize::parse("40%").unwrap(),
        MaxOutputSize {
            value: 40,
            unit: MaxOutputSizeUnit::Percentage,
        }
    );
    assert!(MaxOutputSize::parse("150%").is_err());
    assert!(MaxOutputSize::parse("0KB").is_err());
    assert!(MaxOutputSize::parse("abc").is_err());
}

#[test]
fn calculates_max_output_size_in_bytes_matching_gui() {
    let kb = MaxOutputSize {
        value: 500,
        unit: MaxOutputSizeUnit::Kilobytes,
    };
    assert_eq!(kb.to_bytes(10_000_000), 500 * 1024);

    let pct = MaxOutputSize {
        value: 50,
        unit: MaxOutputSizeUnit::Percentage,
    };
    assert_eq!(pct.to_bytes(1000), 500);
}

#[test]
fn validates_quality_ranges_and_emits_lossless_warnings() {
    let mut opts = CompressionOptions::default();
    assert!(opts.validate().is_ok());

    opts.jpeg_quality = 101;
    assert!(opts.validate().is_err());
    opts.jpeg_quality = 80;

    opts.png_optimization_level = 7;
    assert!(opts.validate().is_err());
    opts.png_optimization_level = 3;

    // GUI parity: lossless + resize or format conversion emits warning
    opts.lossless = true;
    opts.format = OutputFormat::WebP;
    let warnings = opts.warnings();
    assert!(
        warnings
            .iter()
            .any(|w| w.to_lowercase().contains("lossless")),
        "expected lossless warning, got {:?}",
        warnings
    );
}

#[test]
fn parses_resize_shorthand_specs() {
    let dims = ResizeOptions::parse_spec("1920x1080").unwrap();
    assert_eq!(dims.mode, ResizeMode::Dimensions);
    assert_eq!(dims.width, 1920);
    assert_eq!(dims.height, 1080);

    let pct = ResizeOptions::parse_spec("50%").unwrap();
    assert_eq!(pct.mode, ResizeMode::Percentage);
    assert_eq!(pct.width, 50);
    assert_eq!(pct.height, 50);

    let pct_xy = ResizeOptions::parse_spec("50%x75%").unwrap();
    assert_eq!(pct_xy.mode, ResizeMode::Percentage);
    assert_eq!(pct_xy.width, 50);
    assert_eq!(pct_xy.height, 75);

    let long = ResizeOptions::parse_spec("long:1200").unwrap();
    assert_eq!(long.mode, ResizeMode::LongEdge);
    assert_eq!(long.edge, 1200);

    let short = ResizeOptions::parse_spec("short:600").unwrap();
    assert_eq!(short.mode, ResizeMode::ShortEdge);
    assert_eq!(short.edge, 600);

    let w = ResizeOptions::parse_spec("w:800").unwrap();
    assert_eq!(w.mode, ResizeMode::FixedWidth);
    assert_eq!(w.width, 800);

    let h = ResizeOptions::parse_spec("h:600").unwrap();
    assert_eq!(h.mode, ResizeMode::FixedHeight);
    assert_eq!(h.height, 600);
}

#[test]
fn preset_store_has_builtins_and_persists_custom_presets() {
    let dir = TempDir::new().unwrap();
    let store = PresetStore::in_dir(dir.path());

    let builtins = store.list().unwrap();
    assert!(builtins.iter().any(|p| p.name == "web"));
    assert!(builtins.iter().any(|p| p.name == "lossless"));
    assert!(builtins.iter().any(|p| p.name == "thumbnail"));

    let custom = CompressionOptions {
        jpeg_quality: 72,
        format: OutputFormat::WebP,
        webp_quality: 75,
        ..Default::default()
    };
    store
        .save("blog-hero", Some("Hero banner preset".into()), &custom)
        .unwrap();

    let loaded = store.get("blog-hero").unwrap();
    assert_eq!(loaded.options.jpeg_quality, 72);
    assert_eq!(loaded.options.format, OutputFormat::WebP);
    assert_eq!(loaded.options.webp_quality, 75);
    assert!(!loaded.builtin);

    store.delete("blog-hero").unwrap();
    assert!(store.get("blog-hero").is_err());
    // Cannot delete builtin presets
    assert!(store.delete("web").is_err());
}

#[test]
fn config_store_imports_gui_qsettings_ini() {
    let dir = TempDir::new().unwrap();
    let ini_path = dir.path().join("Caesium Image Compressor.conf");
    std::fs::write(
        &ini_path,
        r#"[compression_options]
compression\mode=1
compression\lossless=true
compression\keep_metadata=false
compression\keep_structure=true
compression\jpeg_quality=65
compression\jpeg_chroma_subsampling=420
compression\jpeg_progressive=false
compression\png_quality=70
compression\png_optimization_level=5
compression\webp_quality=55
compression\tiff_method=2
compression\tiff_deflate_level=3
compression\max_output_size=250
compression\max_output_size_unit=1
resize\fit_to=4
resize\width=1200
resize\height=800
resize\size=1600
resize\do_not_enlarge=true
output\output_suffix=_min
output\same_folder_as_input=true
output\skip_if_bigger=false
output\format=3
"#,
    )
    .unwrap();

    let imported = ConfigStore::import_gui_ini(&ini_path).unwrap();
    assert_eq!(imported.mode, CompressionMode::Size);
    assert!(imported.lossless);
    assert!(!imported.keep_metadata);
    assert!(imported.keep_structure);
    assert_eq!(imported.jpeg_quality, 65);
    assert_eq!(
        imported.jpeg_chroma_subsampling,
        ChromaSubsamplingOption::Cs420
    );
    assert!(!imported.jpeg_progressive);
    assert_eq!(imported.png_quality, 70);
    assert_eq!(imported.png_optimization_level, 5);
    assert_eq!(imported.webp_quality, 55);
    assert_eq!(imported.tiff_method, TiffCompressionOption::Deflate);
    assert_eq!(imported.tiff_deflate_level, TiffDeflateLevelOption::Best);
    assert_eq!(imported.max_output_size.value, 250);
    assert_eq!(imported.max_output_size.unit, MaxOutputSizeUnit::Kilobytes);
    assert_eq!(imported.resize.mode, ResizeMode::LongEdge);
    assert_eq!(imported.resize.edge, 1600);
    assert_eq!(imported.suffix, "_min");
    assert!(imported.same_folder_as_input);
    assert!(!imported.skip_if_bigger);
    assert_eq!(imported.format, OutputFormat::WebP);
}
