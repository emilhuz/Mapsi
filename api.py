import json
import requests
from collections import Counter
from math import log, tan, pi
from array import array
from argparse import ArgumentParser
from sys import exit

URL = "https://overpass-api.de/api/interpreter"

def latitudeToMercator(y):
    return log(tan(pi/4 + y*pi/360))*180/pi


def fetch(url, bounds, response_file, headers_file=None):

    body = f"data=[out:json]; way [highway][name] {bounds}; (._;>;); out;"

    resp:requests.Response = requests.post(url, data=body)
    print(resp.status_code)
    with open(response_file, "wb") as f:
        f.write(resp.content)
    if headers_file:
        with open(headers_file, "wb") as f:
            f.write(bytes(str(resp.headers), encoding="utf-8"))

def responseToPointLists(response_file):
    with open(response_file, "rb") as f:
        response = json.loads(f.read())
    els = response["elements"]
    
    c = Counter()
    c.update(el.get("type", None) for el in els)
    print(f"{len(els)} elements: {dict(c)}")

    nodes = {el["id"]:el for el in els if el["type"] == "node"}
    ways = [el for el in els if el["type"] == "way"]

    for w in ways:
        way_nodes = [nodes[nid] for nid in w["nodes"]]
        w["nodes"] = [(n["lon"], latitudeToMercator(n["lat"])) for n in way_nodes]

    ways_by_name = dict()
    for w in ways:
        points = w["nodes"]
        name= w["tags"]["name"]
        present = ways_by_name.get(name, None)
        if present != None:
            present.append(points)
        else:
            ways_by_name[name] = [points]
    print(len(ways_by_name), "streets")
    
    streets, names = [], []
    for name, pointlist in ways_by_name.items():
        names.append(name)
        streets.append(pointlist)
    return streets, names


def responseToStr(response_file, streets_file, names_file):
    streets, names = responseToPointLists(response_file)
    str_streets = []
    for pathlist in streets:
        str_streets.append(" / ".join(
            ", ".join(f"{point[0]} {point[1]}" for point in path) for path in pathlist))
    
    streets_str = "\n".join(str_streets)
    
    with open(streets_file, "w") as f:
        f.write(streets_str)
    with open(names_file, "wb") as f:
        f.write("\n".join(names).encode("utf-8"))


def responseToBinary(response_file, streets_file, names_file):
    streets, names = responseToPointLists(response_file)
    sizes = [len(streets)]
    for pathlist in streets:
        sizes.append(len(pathlist))
        sizes += [len(path) for path in pathlist]
    
    point_coords = []
    for pathlist in streets:
        for path in pathlist:
            for point in path:
                point_coords += point
    
    street_bytes = array("l", sizes).tobytes() + array("f", point_coords).tobytes()
    print(len(street_bytes), "bytes")
    with open(streets_file, "wb") as f:
        f.write(street_bytes)
    with open(names_file, "wb") as f:
        f.write("\n".join(names).encode("utf-8"))


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--bounds", required=True, help="Rectangle containing streets to be fetched as 4 coordinates separated by commas in the order S,W,N,E")
    parser.add_argument("--response", required=True, help="Where to save api response")
    parser.add_argument("--data", required=False, default=None, help="Where to save binary coordinate data")
    parser.add_argument("--names", required=False, default=None, help="Where to save street names")
    args = parser.parse_args()
    try:
        bounds = tuple(float(coord) for coord in args.bounds.split(","))
    except ValueError:
        print("Cannot parse bounds as sequence of four numbers")
        exit(1)
    try:
        fetch(URL, bounds, response_file=args.response, headers_file=None)
    except Exception as e:
        print("Caught exception fetching data:", e)

    try:
        responseToBinary(response_file=args.response, streets_file=args.data, names_file=args.names)
    except Exception as e:
        print("Caught exception parsing data:", e)
    