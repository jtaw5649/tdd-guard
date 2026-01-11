import { describe, it, expect } from 'vitest'
import { detectHookProvider } from './hookFormat'

describe('detectHookProvider', () => {
  it('detects Codex hook payloads via transcript_path', () => {
    const originalEnv = process.env.CLAUDE_PROJECT_DIR
    process.env.CLAUDE_PROJECT_DIR = '/Users/me/projects/demo'

    try {
      const codexHookData = {
        hook_event_name: 'PreToolUse',
        tool_name: 'Write',
        tool_input: {
          file_path: 'src/example.ts',
          content: 'const value = 1',
        },
        session_id: 'session-1',
        transcript_path:
          '/Users/me/.codex/sessions/2026/01/01/rollout-2026-01-01T01-02-03-session-1.jsonl',
      }

      expect(detectHookProvider(codexHookData)).toBe('codex')
    } finally {
      if (originalEnv === undefined) {
        delete process.env.CLAUDE_PROJECT_DIR
      } else {
        process.env.CLAUDE_PROJECT_DIR = originalEnv
      }
    }
  })

  it('detects Codex hook payloads via CODEX_HOME sessions path', () => {
    const originalCodexHome = process.env.CODEX_HOME
    process.env.CODEX_HOME = '/opt/codex-home'

    try {
      const codexHookData = {
        hook_event_name: 'PreToolUse',
        tool_name: 'Write',
        tool_input: {
          file_path: 'src/example.ts',
          content: 'const value = 1',
        },
        session_id: 'session-1',
        transcript_path:
          '/opt/codex-home/sessions/2026/01/01/rollout-2026-01-01T01-02-03-session-1.jsonl',
      }

      expect(detectHookProvider(codexHookData)).toBe('codex')
    } finally {
      if (originalCodexHome === undefined) {
        delete process.env.CODEX_HOME
      } else {
        process.env.CODEX_HOME = originalCodexHome
      }
    }
  })

  it('defaults to Claude when only CLAUDE_PROJECT_DIR is present', () => {
    const originalEnv = process.env.CLAUDE_PROJECT_DIR
    process.env.CLAUDE_PROJECT_DIR = '/Users/me/projects/demo'

    try {
      const claudeHookData = {
        hook_event_name: 'PreToolUse',
        tool_name: 'Write',
        tool_input: {
          file_path: 'src/example.ts',
          content: 'const value = 1',
        },
        session_id: 'session-1',
        transcript_path: '/Users/me/projects/demo/.claude/sessions/rollout.jsonl',
      }

      expect(detectHookProvider(claudeHookData)).toBe('claude')
    } finally {
      if (originalEnv === undefined) {
        delete process.env.CLAUDE_PROJECT_DIR
      } else {
        process.env.CLAUDE_PROJECT_DIR = originalEnv
      }
    }
  })
})
