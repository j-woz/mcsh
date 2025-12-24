BEGIN {
  dontprint=0;
}

/If you believe/ { dontprint = 5; }
  { if (dontprint == 0) { print $0; }
    else { dontprint--; }
  }
