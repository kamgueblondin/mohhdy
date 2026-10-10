## Automated tests for hello.pm (US-057)
use "/pm/lib.pm"
expect file "document.txt" exists
expect $greeting == "bonjour"
expect 2 + 2 == 4
