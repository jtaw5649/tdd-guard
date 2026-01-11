import { spawnSync, execFileSync } from 'node:child_process'
import { join } from 'node:path'
import { existsSync, writeFileSync, copyFileSync, mkdirSync } from 'node:fs'
import type { ReporterConfig, TestScenarios } from '../types'
import { copyTestArtifacts } from './helpers'

export function createCppReporter(): ReporterConfig {
  const cmakeBinary = existsSync('/usr/bin/cmake') ? '/usr/bin/cmake' : 'cmake'
  const artifactDir = 'cpp'
  const testScenarios = {
    singlePassing: 'passing',
    singleFailing: 'failing',
    singleImportError: 'import',
  }

  return {
    name: 'CppReporter',
    testScenarios,
    run: (tempDir, scenario: keyof TestScenarios) => {
      copyTestArtifacts(artifactDir, testScenarios, scenario, tempDir)

      const reporterPath = join(__dirname, '../../cpp')
      const buildDir = join(tempDir, 'cpp-reporter-build')
      const binaryPath = join(tempDir, 'tdd-guard-cpp')

      mkdirSync(buildDir, { recursive: true })

      const configureResult = spawnSync(
        cmakeBinary,
        ['-S', reporterPath, '-B', buildDir, '-DBUILD_TESTING=OFF'],
        {
          stdio: 'pipe',
        }
      )
      if (configureResult.status !== 0) {
        throw new Error(
          `Failed to configure cpp reporter: ${configureResult.stderr.toString()}`
        )
      }

      const buildResult = spawnSync(
        cmakeBinary,
        ['--build', buildDir, '--target', 'tdd-guard-cpp'],
        {
          stdio: 'pipe',
        }
      )
      if (buildResult.status !== 0) {
        throw new Error(
          `Failed to build cpp reporter: ${buildResult.stderr.toString()}`
        )
      }

      copyFileSync(join(buildDir, 'tdd-guard-cpp'), binaryPath)

      const testBuildDir = join(tempDir, 'build')
      mkdirSync(testBuildDir, { recursive: true })

      const configResult = spawnSync(cmakeBinary, ['..'], {
        cwd: testBuildDir,
        stdio: 'pipe',
        encoding: 'utf8',
      })
      if (configResult.status !== 0) {
        throw new Error(
          `Failed to configure cpp test project: ${configResult.stderr || ''}`
        )
      }

      if (scenario === 'singleImportError') {
        const buildTestResult = spawnSync(cmakeBinary, ['--build', '.'], {
          cwd: testBuildDir,
          stdio: 'pipe',
          encoding: 'utf8',
        })

        const combinedOutput = `${configResult.stderr}\n${buildTestResult.stderr}`

        const debugFile = join(tempDir, 'debug-output.txt')
        writeFileSync(debugFile, combinedOutput)

        try {
          execFileSync(
            binaryPath,
            ['--project-root', tempDir, '--passthrough'],
            {
              cwd: tempDir,
              input: combinedOutput,
              stdio: 'pipe',
              encoding: 'utf8',
            }
          )
        } catch (error) {
          console.debug('Reporter processing compilation error:', error)
        }

        return buildTestResult
      }

      const buildTestResult = spawnSync(cmakeBinary, ['--build', '.'], {
        cwd: testBuildDir,
        stdio: 'pipe',
        encoding: 'utf8',
      })

      if (buildTestResult.status !== 0) {
        const debugFile = join(tempDir, 'debug-output.txt')
        writeFileSync(debugFile, buildTestResult.stderr || '')
        return buildTestResult
      }

      const testRunner = join(testBuildDir, 'test_runner')
      const testResult = spawnSync(testRunner, ['--reporter', 'json'], {
        cwd: tempDir,
        stdio: 'pipe',
        encoding: 'utf8',
      })

      const testOutput = `${testResult.stdout}${testResult.stderr}`

      const debugFile = join(tempDir, 'debug-output.txt')
      writeFileSync(debugFile, testOutput)

      try {
        execFileSync(binaryPath, ['--project-root', tempDir, '--passthrough'], {
          cwd: tempDir,
          input: testOutput,
          stdio: 'pipe',
          encoding: 'utf8',
        })
      } catch (error) {
        console.debug('Reporter error:', error)
      }

      return testResult
    },
  }
}
