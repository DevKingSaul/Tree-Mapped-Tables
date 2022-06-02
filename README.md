# Tree Mapped Tables (C++)
### Warning: This project is still in devolpment and is not tested for production yet.

# Benchmarking Results (10k Unique Keys)
```
Old Format:
  Get(): 0.40us per Set (2,500,000 per second)
  Set(): 0.30us per Get (3,333,333 per second)

New Format:
  SetOrdered(): 0.70us per Set (1,428,571 per second)
  GetOrdered(): 0.60us per Get (1,666,666 per second)
```
