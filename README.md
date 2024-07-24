# pkg-config file mangler

The main idea is to take a pkg-config file and merge all the `.private` entries with their non `.private` counterparts.
Why is this needed? Well, if one wants to link statically, this is the easiest way to guarantee you catch all the
dependencies.

But wait you say, why not just use `pkg-config --static --cflags --ldflags --libs`?
Well this may include dependencies, especially from system libraries, which you may not have access to.

The idea here is, copy the libraries you need to a different location, modify them, and then use them instead of the
originals. Of course this is not the way one should do things, but sometimes taking a shortcut is the fastest way to
go...

## Usage

```bash
$ for f in $(ls); do pkgmangle -m -r -i $f; done
```
