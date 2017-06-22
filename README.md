# gstreamer-1.0

To compile
gcc -Wall <name>.cpp -o <output file name> $(pkg-config --cflags --libs gstreamer-1.0)

To launch
./<name>

"is-live" gboolean property
true if the element cannot produce data in PAUSED.

buffering
set pipeline to PAUSED state ; preroll state

