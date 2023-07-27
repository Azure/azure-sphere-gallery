script_filename=$(basename $0)

function log()
{
    level=$1
    message=$2
    echo "::$level file=$script_filename,line=${BASH_LINENO[1]}::$message"
}

function error()
{
    log "error" "$1"
}

function warn()
{
    log "warning" "$1"
}

function debug()
{
    log "debug" "$1"
}

function notice()
{
    log "notice" "$1"
}

function group()
{
    echo "::group::$1"
}

function endgroup()
{
    echo "::endgroup::"
}

azsphere_projects=$(find . -name CMakeLists.txt -print0 | xargs -0 grep -l azsphere_configure_tools)

for p in $azsphere_projects; do
    project_dir=$(dirname $p)

    presets="$project_dir/CMakePresets.json"
    if [ ! -e $presets ]; then
        error "$project_dir: missing CMakePresets.json"
    else
        group "$project_dir"
        cmake --preset ARM-Release $p
        endgroup
    fi

done