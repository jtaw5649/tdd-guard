use std::fs;
use std::process::{Command, Stdio};

use tempfile::TempDir;

#[test]
fn writes_results_to_codex_dir_when_config_present() {
    let temp_dir = TempDir::new().unwrap();
    let project_root = temp_dir.path();

    let codex_dir = project_root.join(".codex");
    fs::create_dir_all(&codex_dir).unwrap();
    fs::write(codex_dir.join("config.toml"), "dummy = true\n").unwrap();

    let mut child = Command::new(env!("CARGO_BIN_EXE_tdd-guard-rust"))
        .args([
            "--project-root",
            project_root.to_string_lossy().as_ref(),
            "--passthrough",
        ])
        .stdin(Stdio::piped())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .unwrap();

    drop(child.stdin.take());
    let status = child.wait().unwrap();
    assert!(status.success());

    let expected_file = project_root
        .join(".codex")
        .join("tdd-guard")
        .join("data")
        .join("test.json");

    assert!(expected_file.exists());
}
