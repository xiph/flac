cd include/FLAC
sed -e 's/@FLaC__SIZE16@/short/g' -e 's/@FLaC__SIZE32@/int/g' -e 's/@FLaC__SIZE64@/long long/g' -e 's/@FLaC__USIZE16@/unsigned short/g' -e 's/@FLaC__USIZE32@/unsigned int/g' -e 's/@FLaC__USIZE64@/unsigned long long/g' ordinals.h.in > ordinals.h
