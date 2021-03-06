#!/bin/bash

# Expose environment from a Bazel target to test with things like MATLAB.

# NOTE: When trying to just execute a desired command, like `bash -i` or
# `matlab`, there are errors about `ioctl` and stuff like that, when using
# `bazel run`...
# (But how does `drake_visualizer` work???)
# According to GitHub issues, no one seems to care about resolving it?

# First, synthesize a fake script_priv to leverage the original environment.
setup_target_env-main() {
    target=$1

    # Use //tools:py_shell as the source file, and add the target such that
    # any necessary dependencies are pulled in.
    local script_dir=$(dirname $script_priv)
    mkdir -p ${script_dir}
    local py_shell=${script_dir}/py_shell.py
    test -L ${py_shell} && rm ${py_shell}
    ln -s ${source_dir_priv}/py_shell.py ${py_shell}

    cat > ${script_dir}/BUILD <<EOF
# Add visibility to isolate this from something like "bazel build //..."?
package(default_visibility = ["//visibility:private"])
# This is used to execute commands using other targets under its environment
# setup.
# NOTE: This is a temporary file. Do not version control!
py_binary(
    name = "py_shell",
    srcs = ["py_shell.py"],
    deps = [
        "${target}",
    ],
    testonly = 1,
)
EOF

    # Easier way to do this?
    # --spawn_strategy=standalone does not seem to do it...

    # Generate environment and export it to a temporary file.
    # How to add additional flags? Like `-c dbg`?
    (
        cd ${script_dir}
        bazel run ${ARGS:-} :py_shell -- \
            bash -c "export -p > $script_priv" \
                || { echo "Error for target: ${target}"; return 1;  }
    ) || return 1;
    # Override PWD
    echo "declare -x PWD=$PWD" >> $script_priv
}

bazel info workspace || return 1

echo $BASH_SOURCE
source_dir_priv=$(cd $(dirname $BASH_SOURCE) && pwd)
script_priv=$(pwd)/.tmp/bazel_env.sh
echo $script_priv
setup_target_env-main "$@" && {
    source $script_priv;
    echo "[ Environment sourced for: ${target} ]"
}
