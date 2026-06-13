"""Inject the firmware version into the build from the current git tag.

Defines FIRMWARE_VERSION (e.g. "v1.2.3" on a tagged build, or
"v1.2.3-4-gabc123" / "abc123" between tags). The on-device self-updater compares
this against the latest GitHub release tag, so a tagged CI build must produce the
same string as the release it is attached to.
"""
import subprocess

Import("env")  # noqa: F821 (provided by PlatformIO/SCons)


def git_version():
    try:
        return (
            subprocess.check_output(
                ["git", "describe", "--tags", "--always", "--dirty"],
                stderr=subprocess.DEVNULL,
            )
            .strip()
            .decode("utf-8")
        )
    except Exception:
        return "dev"


version = git_version()
print("Firmware version: %s" % version)
env.Append(CPPDEFINES=[("FIRMWARE_VERSION", env.StringifyMacro(version))])
