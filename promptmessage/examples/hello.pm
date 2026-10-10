## PromptMessage demo (US-046 examples)
use "/pm/lib.pm"
create file "document.txt" with content "Hello MOHHDY"
set who to "monde"
print $greeting + " " + $who
if system.memory < 90% then print "memoire ok"
repeat 2 times
  append "ligne" to file "journal.txt"
end
when user says "ouvre mes photos" then print "galerie ~/Pictures"
when user says "bonjour" then
  print "salut " + $who
end
