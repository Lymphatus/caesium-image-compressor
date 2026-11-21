mod common;

use caesium_cli::agent::AgentSession;
use caesium_cli::mcp::McpServer;
use image::ImageFormat;
use serde_json::json;
use tempfile::TempDir;

#[test]
fn agent_session_supports_full_stateful_workflow() {
    let dir = TempDir::new().unwrap();
    let in_dir = dir.path().join("in");
    let out_dir = dir.path().join("out");
    let img1 = common::save_fixture(&in_dir, "a.jpg", 180, 120, ImageFormat::Jpeg);
    let img2 = common::save_fixture(&in_dir, "b.png", 100, 100, ImageFormat::Png);

    let mut session = AgentSession::new();

    // 1. Add directory
    let resp = session.handle_json(json!({
        "id": "1",
        "op": "add",
        "paths": [in_dir.to_string_lossy()],
        "recursive": true
    }));
    assert_eq!(resp["ok"], true);
    assert_eq!(resp["id"], "1");
    assert_eq!(resp["result"]["added"], 2);
    assert_eq!(resp["result"]["queue_len"], 2);

    // 2. Apply preset + override option
    let resp = session.handle_json(json!({
        "id": "2",
        "op": "apply_preset",
        "name": "web"
    }));
    assert_eq!(resp["ok"], true);

    let resp = session.handle_json(json!({
        "id": "3",
        "op": "set_options",
        "options": { "skip_if_bigger": false, "jpeg_quality": 65 }
    }));
    assert_eq!(resp["ok"], true);

    // 3. Dry run
    let resp = session.handle_json(json!({
        "id": "4",
        "op": "dry_run",
        "output": out_dir.to_string_lossy()
    }));
    assert_eq!(resp["ok"], true);
    assert_eq!(resp["result"]["items"].as_array().unwrap().len(), 2);
    assert!(!out_dir.exists());

    // 4. Preview
    let resp = session.handle_json(json!({
        "id": "5",
        "op": "preview"
    }));
    assert_eq!(resp["ok"], true);
    assert_eq!(resp["result"]["items"].as_array().unwrap().len(), 2);

    // 5. Remove one, verify list
    let resp = session.handle_json(json!({
        "id": "6",
        "op": "remove",
        "paths": [img2.to_string_lossy()]
    }));
    assert_eq!(resp["ok"], true);
    assert_eq!(resp["result"]["queue_len"], 1);

    // 6. Compress remaining
    let resp = session.handle_json(json!({
        "id": "7",
        "op": "compress",
        "output": out_dir.to_string_lossy(),
        "threads": 1
    }));
    assert_eq!(resp["ok"], true);
    assert_eq!(resp["result"]["summary"]["succeeded"], 1);
    assert!(img1.exists());
    assert!(out_dir.join("a.webp").exists() || out_dir.join("a.jpg").exists());
}

#[test]
fn mcp_server_implements_initialize_tools_resources_and_prompts() {
    let dir = TempDir::new().unwrap();
    let img = common::save_fixture(dir.path(), "pic.png", 120, 90, ImageFormat::Png);
    let out_dir = dir.path().join("out");

    let mut server = McpServer::new();

    // Initialize handshake
    let init = server
        .handle_message(json!({
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "protocolVersion": "2024-11-05",
                "capabilities": {},
                "clientInfo": { "name": "test-agent", "version": "1.0" }
            }
        }))
        .unwrap();
    assert_eq!(init["jsonrpc"], "2.0");
    assert_eq!(init["id"], 1);
    assert_eq!(init["result"]["serverInfo"]["name"], "caesium-cli");

    // Notification returns None
    let notif = server.handle_message(json!({
        "jsonrpc": "2.0",
        "method": "notifications/initialized"
    }));
    assert!(notif.is_none());

    // List tools
    let tools = server
        .handle_message(json!({
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/list",
            "params": {}
        }))
        .unwrap();
    let tool_names: Vec<&str> = tools["result"]["tools"]
        .as_array()
        .unwrap()
        .iter()
        .map(|t| t["name"].as_str().unwrap())
        .collect();
    assert!(tool_names.contains(&"caesium_inspect"));
    assert!(tool_names.contains(&"caesium_preview"));
    assert!(tool_names.contains(&"caesium_compress"));
    assert!(tool_names.contains(&"caesium_convert"));
    assert!(tool_names.contains(&"caesium_batch"));
    assert!(tool_names.contains(&"caesium_presets"));
    assert!(tool_names.contains(&"caesium_retry"));

    // Call caesium_inspect
    let inspect_resp = server
        .handle_message(json!({
            "jsonrpc": "2.0",
            "id": 3,
            "method": "tools/call",
            "params": {
                "name": "caesium_inspect",
                "arguments": {
                    "paths": [img.to_string_lossy()]
                }
            }
        }))
        .unwrap();
    assert_eq!(inspect_resp["result"]["isError"], false);
    assert_eq!(inspect_resp["result"]["structuredContent"]["items"][0]["width"], 120);

    // Call caesium_compress with format conversion to webp
    let comp_resp = server
        .handle_message(json!({
            "jsonrpc": "2.0",
            "id": 4,
            "method": "tools/call",
            "params": {
                "name": "caesium_compress",
                "arguments": {
                    "inputs": [img.to_string_lossy()],
                    "output": out_dir.to_string_lossy(),
                    "format": "webp",
                    "webp_quality": 70,
                    "skip_if_bigger": false
                }
            }
        }))
        .unwrap();
    assert_eq!(comp_resp["result"]["isError"], false);
    assert!(out_dir.join("pic.webp").exists());

    // Resources list & read
    let res_list = server
        .handle_message(json!({
            "jsonrpc": "2.0",
            "id": 5,
            "method": "resources/list",
            "params": {}
        }))
        .unwrap();
    assert!(!res_list["result"]["resources"].as_array().unwrap().is_empty());

    let res_read = server
        .handle_message(json!({
            "jsonrpc": "2.0",
            "id": 6,
            "method": "resources/read",
            "params": { "uri": "caesium://capabilities" }
        }))
        .unwrap();
    assert!(res_read["result"]["contents"][0]["text"]
        .as_str()
        .unwrap()
        .contains("libcaesium"));

    // Prompts list & get
    let prompts = server
        .handle_message(json!({
            "jsonrpc": "2.0",
            "id": 7,
            "method": "prompts/list",
            "params": {}
        }))
        .unwrap();
    assert!(!prompts["result"]["prompts"].as_array().unwrap().is_empty());
}
