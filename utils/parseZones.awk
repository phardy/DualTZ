/^Zone/ {
    print TZNAME" "TZ
    TZNAME=$2
}
/^\t/ {
    TZ=$1
}
