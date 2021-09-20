import numpy as np
import matplotlib.pyplot as plt
import math
import sys

# Extract points from specified file
im = np.loadtxt( sys.argv[1] )

# Display
plt.imshow(im,cmap=plt.cm.flag)
plt.show()
