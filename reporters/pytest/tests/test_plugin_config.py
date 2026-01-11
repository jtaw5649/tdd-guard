"""Test plugin configuration functionality"""
from pathlib import Path
from tdd_guard_pytest.pytest_reporter import (
    TDDGuardPytestPlugin,
    DEFAULT_DATA_DIR,
    DEFAULT_CODEX_DATA_DIR,
)
from .helpers import create_config


def test_plugin_accepts_config_parameter():
    """Test that TDDGuardPytestPlugin can be initialized with a config parameter"""
    # Create a minimal config object
    class MinimalConfig:
        pass
    
    config = MinimalConfig()
    
    # Plugin should accept config parameter without error
    plugin = TDDGuardPytestPlugin(config)
    
    # Default behavior - should still use default storage dir
    assert plugin.storage_dir == DEFAULT_DATA_DIR


def test_plugin_uses_configured_project_root():
    """Test that plugin uses tdd_guard_project_root from config"""
    project_root = Path("/test/project")
    cwd = project_root / "subdir"
    
    config = create_config(str(project_root))
    plugin = TDDGuardPytestPlugin(config, cwd=cwd)
    
    # Plugin should use the configured directory
    expected_storage = project_root / DEFAULT_DATA_DIR
    assert plugin.storage_dir == expected_storage


def test_plugin_uses_codex_project_dir_from_env(monkeypatch):
    """Test that CODEX_PROJECT_DIR overrides config when valid."""
    project_root = Path("/codex/project")
    cwd = project_root / "src"

    monkeypatch.setenv("CODEX_PROJECT_DIR", str(project_root))

    class MinimalConfig:
        pass

    plugin = TDDGuardPytestPlugin(MinimalConfig(), cwd=cwd)

    expected_storage = project_root / DEFAULT_CODEX_DATA_DIR
    assert plugin.storage_dir == expected_storage


def test_plugin_uses_codex_config_in_project_root(tmp_path):
    """Test that .codex/config.toml switches storage dir to Codex."""
    project_root = tmp_path / "project"
    codex_dir = project_root / ".codex"
    cwd = project_root / "src"

    codex_dir.mkdir(parents=True)
    cwd.mkdir(parents=True)
    (codex_dir / "config.toml").write_text("")

    config = create_config(str(project_root))
    plugin = TDDGuardPytestPlugin(config, cwd=cwd)

    expected_storage = project_root / DEFAULT_CODEX_DATA_DIR
    assert plugin.storage_dir == expected_storage
