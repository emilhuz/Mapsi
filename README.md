# Mapsi

Interactive satellite image and street rendering implementation in C++

![screenshot](https://github.com/emilhuz/Mapsi/raw/master/docs/img.png)

## Data

To download streets inside another area, use the fetcher:

```sh
python -m api.py --bounds S,W,N,E --response Data/resp.json --data Data/streets.bin --names Data/names.txt
```

S,W must be the decimal coordinates (latitude and longitude) of the southwest corner and N,E of the northeast.

## Requirements

Compile glew, glfw and freetype or download precompiled libraries and link to them.

C++ 11 or higher is required.
