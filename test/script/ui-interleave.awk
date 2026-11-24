# Checks a TEST:UI session log (cat -v form) for interleaving:
# every line of program output must come right after a prompt
# and right before the next prompt.  Exits 1 if any output line is
# not surrounded by prompts, or if there is no output line at all.
#
# Usage: awk -f test/script/ui-interleave.awk SESSION.out

{ line[NR] = $0 }

function is_prompt(s) { return s ~ /mcsh> /  }
function is_output(s) { return s ~ /\^M$/ && !is_prompt(s) }

END {
  bad = 0
  count = 0
  for (i = 1; i <= NR; i++)
  {
    if (! is_output(line[i])) continue
    count++
    if (i == 1 || ! is_prompt(line[i-1]))
    {
      print "output not preceded by a prompt, line " i ": " line[i]
      bad = 1
    }
    if (i == NR || ! is_prompt(line[i+1]))
    {
      print "output not followed by a prompt, line " i ": " line[i]
      bad = 1
    }
  }
  if (count == 0)
  {
    print "no output lines found"
    bad = 1
  }
  exit bad
}
