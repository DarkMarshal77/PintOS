# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(cache-w-no-r) begin
(cache-w-no-r) create "x"
(cache-w-no-r) open "x"
(cache-w-no-r) writing "x"
(cache-w-no-r) close "x"
(cache-w-no-r) end
EOF
pass;
