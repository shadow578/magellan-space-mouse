# based on https://community.platformio.org/t/how-to-build-got-revision-into-binary-for-version-output/15380/5
import subprocess
Import("env")

def get_version_build_flags():
    # ensure git is available and this is a git repo
    if subprocess.call(["git", "status"], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL) != 0:
        raise Exception("cannot get version info: not a git repository or git not installed")

    # get branch name
    # $ git rev-parse --abbrev-ref HEAD
    branch = subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], text=True).strip()

    # get commit hash
    # $ git rev-parse --short HEAD
    commit_hash = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], text=True).strip()

    # check if dirty
    # $ git diff-index --quiet HEAD --
    # -> exit code 0 if clean, 1 if dirty
    is_dirty = subprocess.call(["git", "diff-index", "--quiet", "HEAD", "--"], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)

    # get origin url
    # $ git config --get remote.origin.url
    origin_url = subprocess.check_output(["git", "config", "--get", "remote.origin.url"], text=True).strip()

    version_string = f"{origin_url}/commit/{commit_hash} on branch {branch}{' (dirty working tree)' if is_dirty else ''} (PIOENV={env['PIOENV']})"

    print(f"resolved version: '{version_string}'")
    return [
        f"-D GIT_VERSION_STRING='\"{version_string}\"'",

        f"-D GIT_BRANCH='\"{branch}\"'",
        f"-D GIT_COMMIT_HASH='\"{commit_hash}\"'",
        f"-D GIT_IS_DIRTY={is_dirty}",
        f"-D GIT_ORIGIN_URL='\"{origin_url}\"'"
    ]

# update build flags
env.Append(
    BUILD_FLAGS=[
        *get_version_build_flags()
    ]
)