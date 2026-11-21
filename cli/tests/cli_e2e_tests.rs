mod common;

use assert_cmd::Command;
use image::ImageFormat;
use serde_json::Value;
use tempfile::TempDir;

fn cli() -> Command {
    Command::cargo_bin("caesium-cli").unwrap()
}

#[test]
fn capabilities_and_schema_emit_valid_machine_readable_json() {
    let out = cli()
        .args(["capabilities", "--json"])
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    let v: Value = serde_json::from_slice(&out).unwrap();
    assert_eq!(v["ok"], true);
    assert_eq!(v["result"]["engine"]["name"], "libcaesium");
    assert!(v["result"]["formats"]["input"]
        .as_array()
        .unwrap()
        .iter()
        .any(|f| f == "webp"));

    let schema_out = cli()
        .args(["schema", "options"])
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    let sv: Value = serde_json::from_slice(&schema_out).unwrap();
    assert!(sv["properties"]["jpeg_quality"].is_object());
}

#[test]
fn compress_subcommand_json_and_ndjson_work_end_to_end() {
    let dir = TempDir::new().unwrap();
    let in_dir = dir.path().join("in");
    let out_dir = dir.path().join("out");
    common::save_fixture(&in_dir, "a.jpg", 200, 150, ImageFormat::Jpeg);
    common::save_fixture(&in_dir, "b.png", 120, 120, ImageFormat::Png);

    // JSON mode
    let out = cli()
        .args([
            "compress",
            in_dir.to_str().unwrap(),
            "-o",
            out_dir.to_str().unwrap(),
            "-q",
            "65",
            "--no-skip-if-bigger",
            "--json",
        ])
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    let v: Value = serde_json::from_slice(&out).unwrap();
    assert_eq!(v["ok"], true);
    assert_eq!(v["result"]["summary"]["total"], 2);
    assert_eq!(v["result"]["summary"]["failed"], 0);
    assert!(out_dir.join("a.jpg").exists());
    assert!(out_dir.join("b.png").exists());

    // NDJSON streaming mode
    let out_nd = dir.path().join("out_nd");
    let nd = cli()
        .args([
            "compress",
            in_dir.to_str().unwrap(),
            "-o",
            out_nd.to_str().unwrap(),
            "--format",
            "webp",
            "--no-skip-if-bigger",
            "--ndjson",
        ])
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    let lines: Vec<Value> = String::from_utf8(nd)
        .unwrap()
        .lines()
        .map(|l| serde_json::from_str(l).unwrap())
        .collect();
    assert!(lines.iter().any(|l| l["event"] == "start"));
    assert!(lines.iter().filter(|l| l["event"] == "item").count() == 2);
    assert!(lines.iter().any(|l| l["event"] == "complete"));
}

#[test]
fn overwrite_guard_exits_with_code_4_without_flag() {
    let dir = TempDir::new().unwrap();
    let img = common::save_fixture(dir.path(), "guard.jpg", 120, 120, ImageFormat::Jpeg);

    let output = cli()
        .args(["compress", img.to_str().unwrap(), "--same-folder", "--json"])
        .assert()
        .code(4)
        .get_output()
        .stdout
        .clone();
    let v: Value = serde_json::from_slice(&output).unwrap();
    assert_eq!(v["ok"], false);
    assert_eq!(v["error"]["code"], "OVERWRITE_REFUSED");
}

#[test]
fn stdin_to_stdout_streaming_pipeline_works() {
    let dir = TempDir::new().unwrap();
    let img = common::save_fixture(dir.path(), "pipe.png", 96, 96, ImageFormat::Png);
    let bytes = std::fs::read(&img).unwrap();

    let out = cli()
        .args([
            "compress",
            "-",
            "-o",
            "-",
            "--format",
            "webp",
            "--webp-quality",
            "70",
        ])
        .write_stdin(bytes)
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    assert!(!out.is_empty());
    assert_eq!(infer::get(&out).unwrap().mime_type(), "image/webp");
}

#[test]
fn batch_manifest_runs_multiple_targets() {
    let dir = TempDir::new().unwrap();
    let src = common::save_fixture(dir.path(), "hero.png", 300, 200, ImageFormat::Png);
    let out_webp = dir.path().join("webp");
    let out_thumb = dir.path().join("thumb");
    let spec_path = dir.path().join("batch.json");

    let spec = serde_json::json!({
        "tasks": [
            {
                "name": "web-webp",
                "inputs": [src.to_string_lossy()],
                "output": out_webp.to_string_lossy(),
                "options": {
                    "format": "webp",
                    "webp_quality": 75,
                    "skip_if_bigger": false
                }
            },
            {
                "name": "thumb-jpg",
                "inputs": [src.to_string_lossy()],
                "output": out_thumb.to_string_lossy(),
                "options": {
                    "format": "jpg",
                    "jpeg_quality": 70,
                    "skip_if_bigger": false,
                    "resize": {
                        "mode": "long_edge",
                        "edge": 100,
                        "do_not_enlarge": true
                    }
                }
            }
        ]
    });
    std::fs::write(&spec_path, serde_json::to_string_pretty(&spec).unwrap()).unwrap();

    let out = cli()
        .args(["batch", spec_path.to_str().unwrap(), "--json"])
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    let v: Value = serde_json::from_slice(&out).unwrap();
    assert_eq!(v["ok"], true);
    assert!(out_webp.join("hero.webp").exists());
    assert!(out_thumb.join("hero.jpg").exists());
    assert_eq!(
        image::image_dimensions(out_thumb.join("hero.jpg")).unwrap(),
        (100, 67)
    );
}

#[test]
fn inspect_preview_and_doctor_commands_succeed() {
    let dir = TempDir::new().unwrap();
    let img = common::save_fixture(dir.path(), "check.jpg", 180, 120, ImageFormat::Jpeg);

    let ins = cli()
        .args(["inspect", img.to_str().unwrap(), "--json"])
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    let iv: Value = serde_json::from_slice(&ins).unwrap();
    assert_eq!(iv["result"]["items"][0]["width"], 180);

    let prev = cli()
        .args(["preview", img.to_str().unwrap(), "-q", "50", "--json"])
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    let pv: Value = serde_json::from_slice(&prev).unwrap();
    assert!(
        pv["result"]["items"][0]["compressed_size"]
            .as_u64()
            .unwrap()
            > 0
    );

    let doc = cli()
        .args(["doctor", "--json"])
        .assert()
        .success()
        .get_output()
        .stdout
        .clone();
    let dv: Value = serde_json::from_slice(&doc).unwrap();
    assert_eq!(dv["ok"], true);
}
