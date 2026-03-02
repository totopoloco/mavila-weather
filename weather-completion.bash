# Bash completion for weather CLI
# Install: source this file or copy to /etc/bash_completion.d/weather

_weather_completions() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local opts="--latitude --longitude --city --country --hourly --daily --location --sources --help"

    COMPREPLY=($(compgen -W "$opts" -- "$cur"))
}

complete -F _weather_completions weather
