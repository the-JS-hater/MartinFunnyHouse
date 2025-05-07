
def genAABB(l):
    min_x = float("inf")
    min_y = float("inf")
    min_z = float("inf")
    max_x = -float("inf")
    max_y = -float("inf")
    max_z = -float("inf")
    
    for x,y,z in l:
        min_x = min(x, min_x) 
        min_y = min(y, min_y)
        min_z = min(z, min_z)
        max_x = max(x, max_x) 
        max_y = max(y, max_y)
        max_z = max(z, max_z)
    
    print(f"AABB limits\nmin_x: {min_x}, min_y: {min_y}, min_z: {min_z}")
    print(f"AABB limits\nmax_x: {max_x}, max_y: {max_y}, max_z: {max_z}")
    

def clean(f):
    return [
        list(map(float, l.split()[1:]))
        for l in f
        if "v " in l
    ]


if __name__ == "__main__":
    f = open(0).read().splitlines()    
    genAABB(clean(f))
