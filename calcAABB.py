
def genAABB(l):
    coord_map = {
        "min_x" : None,
        "min_y" : None,
        "min_z" : None,
        "max_x" : None,
        "max_y" : None,
        "max_z" : None,
    }
    min_x = float("inf")
    min_y = float("inf")
    min_z = float("inf")
    max_x = -float("inf")
    max_y = -float("inf")
    max_z = -float("inf")
    
    for x,y,z in l:
        if x < min_x: min_x = x; coord_map["min_x"] = (x,y,z) 
        if y < min_y: min_y = x; coord_map["min_y"] = (x,y,z) 
        if z < min_z: min_z = x; coord_map["min_z"] = (x,y,z) 
        if x > max_x: max_x = x; coord_map["max_x"] = (x,y,z) 
        if y > max_y: max_y = x; coord_map["max_y"] = (x,y,z) 
        if z > max_z: max_z = x; coord_map["max_z"] = (x,y,z) 
    
    print(coord_map)
    for key,val in coord_map.items(): print(f"{key} : {val}")

def clean(f):
    return [
        list(map(float, l.split()[1:]))
        for l in f
        if "v " in l
    ]


if __name__ == "__main__":
    f = open(0).read().splitlines()    
    genAABB(clean(f))
