value = 0

def simple():
    global value
    value += 1
    # Called by MATLAB, which is called by C
    print "py: Simple {}".format(value)
