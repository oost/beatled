# Beat Server

### Compilation

1. Download submodule dependencies.

   ```
   git sumbodule update
   ```

   1. Note that you manually need to downgrade fmt to 8.1.1

      ```
      cd server/external/fmt
      git checkout 8.1.1
      ```

2. Build builder image:

```
utils/build-docker-builder.sh
```

3. Build Raspberry Pi executable:

```
utils/build-beatled-server.sh
```

## Requirements

- On MacOS
  - `brew install pkg-config cmake libtool automake autoconf autoconf-archive`

### Ideas

- Use [Ableton Link](https://github.com/Ableton/link/tree/master) to synchronize all the devices
