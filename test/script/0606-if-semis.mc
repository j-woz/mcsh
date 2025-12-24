# TEST:SKIP
if { code 1 ; condition 1} {
   if { code 2  ; condition 2} {
     code 3 ; exit
   }
   code 4
}
