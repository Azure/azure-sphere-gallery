script_filename=$(basename $0)

function log()
{
    level=$1
    message=$2
    echo "::$level file=$script_filename,line=${BASH_LINENO[1]}::$message"
}

function warn()
{
    log "warning" "$1"
}

azsphere_projects=$(find . -name CMakeLists.txt -print0 | xargs -0 grep -l azsphere_configure_tools)

for p in $azsphere_projects; do
    project_dir=$(dirname $p)
    echo "::group $project_dir"

    presets="$project_dir/CMakePreset.json"
    if [ ! -e $presets ]; then
        warn "Missing $presets - skipping"
    fi

    echo "::endgroup"
done