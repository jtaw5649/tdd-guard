export type HookProvider = 'claude' | 'codex'

export function detectHookProvider(hookData: unknown): HookProvider {
  if (!hookData || typeof hookData !== 'object') {
    return 'claude'
  }

  const data = hookData as { transcript_path?: unknown }

  if (isCodexTranscriptPath(data.transcript_path)) {
    return 'codex'
  }

  return 'claude'
}

function isCodexTranscriptPath(value: unknown): boolean {
  if (typeof value !== 'string') {
    return false
  }

  const normalized = value.replace(/\\/g, '/')
  if (normalized.includes('/.codex/sessions/')) {
    return true
  }

  const codexHome = process.env.CODEX_HOME
  if (!codexHome) {
    return false
  }

  const codexHomeNormalized = codexHome.replace(/\\/g, '/').replace(/\/$/, '')
  return normalized.startsWith(`${codexHomeNormalized}/sessions/`)
}
