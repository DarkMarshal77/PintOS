# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(cache-write) begin
(cache-write) create "x"
(cache-write) open "x"
(cache-write) writing "x"
(cache-write) close "x"
(cache-write) open "x" for verification
(cache-write) verified contents of "x"
(cache-write) close "x"
(cache-write) end
EOF
pass;
