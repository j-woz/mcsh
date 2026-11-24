# TEST:EXPECT: type file
# TEST:EXPECT: noerr 0
# TEST:EXPECT: err ENOENT
# TEST:EXPECT: err ENOTDIR
f=(( os stat test/script/3802-os-stat-1.mc ))
print type $f[type]
print noerr $+f[error]
m=(( os stat no/such/file ))
print err $m[error]
n=(( os stat test/script/3802-os-stat-1.mc/x ))
print err $n[error]
