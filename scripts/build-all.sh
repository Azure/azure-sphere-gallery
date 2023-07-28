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

exclude_paths="MT3620_M4_Sample_Code;AzureSphereDevX/examples;"

excludes=$(echo $exclude_paths | tr ";" "\n")

for p in $azsphere_projects; do
    skip=""
    for exclude in $excludes; do
        if [[ $p =~ "$exclude" ]]; then
            skip="true"
        fi
    done

    if [[ $skip == "true" ]]; then
        notice "Skipping $p - excluded"
        continue
    fi

    project_dir=$(dirname $p)

    presets="$project_dir/CMakePresets.json"
    if [ ! -e $presets ]; then
        error "$project_dir: missing CMakePresets.json"
    else
        group "$project_dir"
        if cmake --preset ARM-Release $p; then
            cmake --build $project_dir/out/ARM-Release
        else
            error "$project_dir: Configuration failed"
        fi
        endgroup
    fi

done