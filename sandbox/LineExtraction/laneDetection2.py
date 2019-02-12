import numpy as np
import pandas as pd
import cv2
import os
import glob
import matplotlib.pyplot as plt
import pickle
import math
from sklearn.metrics import r2_score
from sklearn.metrics import mean_squared_error
import imutils
import time

class contourObject:

	def __init__(self, contour, extLeft, extRight, midpoint):
		self.contour = contour
		self.extLeft = extLeft
		self.extRight = extRight
		self.midpoint = midpoint 

# Create a VideoCapture object and read from input file 
cap = cv2.VideoCapture('videos/vid1.mp4') 
   
# Check if camera opened successfully 
if (cap.isOpened()== False):  
  print("Error opening video  file") 
   
# Read until video is completed 
while(cap.isOpened()): 
	  
  # Capture frame-by-frame 
  ret, im = cap.read() 
  if ret == True: 

  	cv2.imshow("", im)
	cv2.waitKey(0)
	t0 = time.time()
	height = np.size(im, 0)
	width = np.size(im, 1)
	# plt.imshow(im)
	# plt.show()
	imgray = cv2.cvtColor(im, cv2.COLOR_BGR2GRAY)

	# kernel = np.ones((5,5), np.uint8)
	# img_dilation = cv2.dilate(imgray, kernel, iterations=4)
	# # cv2.imshow('', img_dilation)
	# # cv2.waitKey(0)
	# img_erosion = cv2.erode(img_dilation, kernel, iterations=3)
	# cv2.imshow('', img_erosion)
	# cv2.waitKey(0)
	ret, thresh = cv2.threshold(imgray, 127, 255, 0)
	im2, contours, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
	# for cnt in contours:
	#     cv2.drawContours(im,[cnt],0,(0,255,0),2)

	# cv2.imshow('output',im)
	# cv2.waitKey(0)

	x = 0;
	noElementInList2 = True


	array1 = []
	array2 = []


	arr1XMid = 0
	arr1YMid = 0
	arr1ExtLeft = 0#tuple(c[c[:, :, 0].argmin()][0])
	arr1ExtRight = 0#tuple(c[c[:, :, 0].argmax()][0])
	arr2XMid = 0
	arr2YMid = 0
	arr2ExtLeft = 0#tuple(c[c[:, :, 0].argmin()][0])
	arr2ExtRight = 0#tuple(c[c[:, :, 0].argmax()][0])


	array1x = []
	array1y = []
	array2x = []
	array2y = []

	# contour_min_xs = []
	# valid_contours = []

	def sort_contours(cnts, method="left-to-right"):
		# initialize the reverse flag and sort index
		reverse = False
		i = 0
	 
		# handle if we need to sort in reverse
		if method == "right-to-left" or method == "bottom-to-top":
			reverse = True
	 
		# handle if we are sorting against the y-coordinate rather than
		# the x-coordinate of the bounding box
		if method == "top-to-bottom" or method == "bottom-to-top":
			i = 1
	 
		# construct the list of bounding boxes and sort them from top to
		# bottom
		boundingBoxes = [cv2.boundingRect(c) for c in cnts]
		(cnts, boundingBoxes) = zip(*sorted(zip(cnts, boundingBoxes),
			key=lambda b:b[1][i], reverse=reverse))
	 
		# return the list of sorted contours and bounding boxes
		return (cnts, boundingBoxes)

	boundingBoxes = [cv2.boundingRect(c) for c in contours]
	(contours, boundingBoxes) = zip(*sorted(zip(contours, boundingBoxes),
		key=lambda b:b[1][0], reverse=False))
	# for i in range(len(contours)):
	#     if cv2.arcLength(contours[i], True) < 67 or cv2.contourArea(contours[i]) < 34.0:
	#         continue
	#     valid_contours.append(contours[i])
	# for i in range(len(valid_contours)):
	#     smallest_x = im.shape[1]    
	#     for j in range(len(contours[i])):
	#         x = contours[i][j][0][0]
	#         print(x)
	#         if x < smallest_x:
	#             smallest_x = x
	#     contour_min_xs.append(smallest_x)
	# inds = np.argsort(contour_min_xs)
	# contours = list(np.array(contours)[inds])

	for cnt in contours:
		# if cv2.arcLength(cnt, True) < 67 or cv2.contourArea(cnt) < 34.0:
		#  	continue
		currExtLeft = tuple(cnt[cnt[:, :, 0].argmin()][0])
		currExtRight = tuple(cnt[cnt[:, :, 0].argmax()][0])
		xVal = 0
		yVal = 0
		points = 0
		for pointA in cnt:
			xVal = xVal + pointA[0][0]
			yVal = yVal + pointA[0][1]
			points = points+1
		xVal = xVal/points
		yVal = yVal/points
		cv2.circle(im, (xVal, yVal), 7, (255, 255, 255), -1)
		if x == 0:
			# array1.append(cnt)
			# arr1XMid = xVal
			# arr1YMid = yVal
			# arr1ExtRight = currExtRight
			# arr1ExtLeft = currExtLeft
			array1.append(contourObject(cnt, currExtLeft, currExtRight, [xVal, yVal]))

			# cv2.drawContours(im, [cnt],0,(0,255,0),2)
			# array1.append([cnt])
		else:
			if noElementInList2:
				# print(((arr1XMid - xVal) ** 2 + (arr1YMid - yVal) ** 2) ** 0.5)
				lowestDist = float("inf")
				for cntObj in array1:
					distListArr1 = [((cntObj.midpoint[0] - xVal) ** 2 + (cntObj.midpoint[1] - yVal) ** 2) ** 0.5 , 
					((cntObj.extRight[0] - currExtLeft[0]) ** 2 + (cntObj.extRight[1] - currExtLeft[1]) ** 2) ** 0.5, 
					((cntObj.extLeft[0] - currExtRight[0]) ** 2 + (cntObj.extLeft[1] - currExtRight[1]) ** 2) ** 0.5]
					lowestDist = min([lowestDist, min(distListArr1)])
				print(lowestDist)
				#print(min(distListArr1))
				#print(((arr1ExtRight[0] - currExtLeft[0]) ** 2 + (arr1ExtRight[1] - currExtLeft[1]) ** 2) ** 0.5)
				#print(arr1ExtRight)
				
				if  lowestDist > 185:
					# print(((arr1XMid - xVal) ** 2 + (arr1YMid - yVal) ** 2) ** 0.5)
					array2.append(contourObject(cnt, currExtLeft, currExtRight, [xVal, yVal]))
					# arr2XMid = xVal
					# arr2YMid = yVal
					# arr2ExtRight = currExtRight
					# arr2ExtLeft = currExtLeft
					noElementInList2 = False
				else:
					array1.append(contourObject(cnt, currExtLeft, currExtRight, [xVal, yVal]))
					# arr1XMid = xVal
					# arr1YMid = yVal
					# arr1ExtRight = currExtRight
					# arr1ExtLeft = currExtLeft
			else:

				lowestDist1 = float("inf")
				for cntObj in array1:
					distListArr1 = [((cntObj.midpoint[0] - xVal) ** 2 + (cntObj.midpoint[1] - yVal) ** 2) ** 0.5 , 
					((cntObj.extRight[0] - currExtLeft[0]) ** 2 + (cntObj.extRight[1] - currExtLeft[1]) ** 2) ** 0.5, 
					((cntObj.extLeft[0] - currExtRight[0]) ** 2 + (cntObj.extLeft[1] - currExtRight[1]) ** 2) ** 0.5]
					lowestDist1 = min([lowestDist1, min(distListArr1)])
				# distListArr1 = [((arr1XMid - xVal) ** 2 + (arr1YMid - yVal) ** 2) ** 0.5 , 
				# ((arr1ExtRight[0] - currExtLeft[0]) ** 2 + (arr1ExtRight[1] - currExtLeft[1]) ** 2) ** 0.5, 
				# ((arr1ExtLeft[0] - currExtRight[0]) ** 2 + (arr1ExtLeft[1] - currExtRight[1]) ** 2) ** 0.5]


				lowestDist2 = float("inf")
				for cntObj in array2:
					distListArr2 = [((cntObj.midpoint[0] - xVal) ** 2 + (cntObj.midpoint[1] - yVal) ** 2) ** 0.5 , 
					((cntObj.extRight[0] - currExtLeft[0]) ** 2 + (cntObj.extRight[1] - currExtLeft[1]) ** 2) ** 0.5, 
					((cntObj.extLeft[0] - currExtRight[0]) ** 2 + (cntObj.extLeft[1] - currExtRight[1]) ** 2) ** 0.5]
					lowestDist2 = min([lowestDist2, min(distListArr2)])
				# distListArr2 = [((arr2XMid - xVal) ** 2 + (arr2YMid - yVal) ** 2) ** 0.5 , 
				# ((arr2ExtRight[0] - currExtLeft[0]) ** 2 + (arr2ExtRight[1] - currExtLeft[1]) ** 2) ** 0.5, 
				# ((arr2ExtLeft[0] - currExtRight[0]) ** 2 + (arr2ExtLeft[1] - currExtRight[1]) ** 2) ** 0.5]
				# print(((arr1XMid - xVal) ** 2 + (arr1YMid - yVal) ** 2) ** 0.5)

				if lowestDist1 > lowestDist2:
					array2.append(contourObject(cnt, currExtLeft, currExtRight, [xVal, yVal]))
					# arr2XMid = xVal
					# arr2YMid = yVal
					# arr2ExtRight = currExtRight
					# arr2ExtLeft = currExtLeft
					# print("Here")
				else:
					array1.append(contourObject(cnt, currExtLeft, currExtRight, [xVal, yVal]))
					# arr1XMid = xVal
					# arr1YMid = yVal
					# arr1ExtRight = currExtRight
					# arr1ExtLeft = currExtLeft

		x = x + 1

	# cv2.drawContours(im, array1[0],0,(0,255,0),2)
	# cv2.drawContours(im, array2[0],0,(255,0,0),2)
	# print(array1)
	# print(array2)
	# cv2.imshow("", im)
	# cv2.waitKey(0);

	# nearest_dist = 100000000000000000
	# print(contours[0][0].size)
	# xVal = 0
	# yVal = 0
	# for pointA in contours[1]:
	# 	xVal = xVal + pointA[0][0]
	# 	yVal = yVal + pointA[0][1]
	# print(xVal/887)
	# print(yVal/887)
	# print(array1)
	# print(len(array1))
	# print(len(array2))
	# THIS WORKS
	print("-----------------------------------------------")
	for cntObj in array1:
		cv2.drawContours(im, [cntObj.contour],0,(255,0,0),2)
		# print(cnt.size)
		for pointA in cntObj.contour:
			array1x.append(pointA[0][0])
			array1y.append(pointA[0][1])
			# print(cnt)
			# array1.append([cnt])
	for cntObj in array2:
		cv2.drawContours(im, [cntObj.contour],0,(0,0,255),2)
		for pointA in cntObj.contour:
			array2x.append(pointA[0][0])
			array2y.append(pointA[0][1])
			# print(cnt)
			# array1.append([cnt])




	# cv2.drawContours(im, [contours[3]],0,(0,255,0),2)
	# cv2.imshow("", im)
	# cv2.waitKey(0);

	# print(len(array1x))
	# print(array1y)
	l_funcx_points = []
	l_funcx_ypred = []

	l_funcy_points = []
	l_funcy_xpred = []

	r_funcx_points = []
	r_funcx_ypred = []

	r_funcy_points = []
	r_funcy_xpred = []


	# if len(array1x) > 0:
	# 	#array1x = array1x.reshape(-1, 1)
	# 	model = make_pipeline(PolynomialFeatures(2), Ridge())
	# 	model = model.fit(array1x, array1y)
	# 	print(model)
	# 	model.named_steps['linear'].coef_
	# 	y_plot = model.predict(array1x)
	# 	print(y_plot)


	if len(array1x) > 0:

		#fit y = x^2
		l = np.polyfit(array1x, array1y, 2)  
		x1 = np.linspace(0, width, 400)
		for i in x1:
			# y1 = l[0] * i**5 + l[1] * i**4 + l[2] * i**3 + l[3] * i**2 + l[4] * i + l[5] 
			y1 = l[0] * i**1 + l[1] * i**1 + l[2]
			# y1 = l[0] * i + l[1]
			if y1 < height and y1 > 0:
				l_funcx_points.append([i, y1]) 
		for i in array1x:
			y1 = l[0] * i**2 + l[1] * i**1 + l[2]
			# y1 = l[0] * i**5 + l[1] * i**4 + l[2] * i**3 + l[3] * i**2 + l[4] * i + l[5]
			# y1 = l[0] * i + l[1]
			l_funcx_ypred.append(y1)


		#fit x = y^2 -- for hairpin
		l = np.polyfit(array1y, array1x, 2)  #Switched for hairpin
		y1 = np.linspace(0, height, 400)
		for i in y1:
			x1 = l[0] * i**2 + l[1] * i**1 + l[2]
			# x1 = l[0] * i**5 + l[1] * i**4 + l[2] * i**3 + l[3] * i**2 + l[4] * i + l[5]
			# x1 = l[0] * i + l[1]
			if x1 < width and x1 > 0:
				l_funcy_points.append([x1, i]) #Switched for hairpin
		for i in array1y:
			x1 = l[0] * i**2 + l[1] * i**1 + l[2]
			# x1 = l[0] * i**5 + l[1] * i**4 + l[2] * i**3 + l[3] * i**2 + l[4] * i + l[5]
			# x1 = l[0] * i + l[1]
			l_funcy_xpred.append(x1)

		# print(array1y)
		# print(l_funcx_ypred)
		# print(mean_squared_error(array1y, l_funcx_ypred))
		# print(mean_squared_error(array1x, l_funcy_xpred))
		# l_points = np.array(l_funcx_points, dtype=np.int32)
		# cv2.polylines(im, [l_points], 0, (255,0,0))
		# l_points = np.array(l_funcy_points, dtype=np.int32)
		# cv2.polylines(im, [l_points], 0, (0,255,0))

		if mean_squared_error(array1y, l_funcx_ypred) < mean_squared_error(array1x, l_funcy_xpred):
			l_points = np.array(l_funcx_points, dtype=np.int32)
			cv2.polylines(im, [l_points], 0, (255,0,0))
		else:
			l_points = np.array(l_funcy_points, dtype=np.int32)
			cv2.polylines(im, [l_points], 0, (255,0,0))


	if len(array2x) > 0:

		#fit y = x^2
		r = np.polyfit(array2x, array2y, 2)
		x2 = np.linspace(0, width, 400)
		for i in x2:
			y2 = r[0] * i**2 + r[1] * i**1 + r[2]
			# y2 = r[0] * i**5 + r[1] * i**4 + r[2] * i**3 + r[3] * i**2 + r[4] * i + r[5]
			# y2 = r[0] * i + r[1]
			if y2 < height and y2 > 0:
				r_funcx_points.append([i, y2])
		for i in array2x:
			y2 = r[0] * i**2 + r[1] * i**1 + r[2]
			# y2 = r[0] * i**5 + r[1] * i**4 + r[2] * i**3 + r[3] * i**2 + r[4] * i + r[5]
			# y2 = r[0] * i + r[1]
			r_funcx_ypred.append(y2)

		#fit x = y^2
		r = np.polyfit(array2y, array2x, 2)
		y2 = np.linspace(0, width, 400)
		for i in y2:
			x2 = r[0] * i**2 + r[1] * i**1 + r[2]
			# x2 = r[0] * i**5 + r[1] * i**4 + r[2] * i**3 + r[3] * i**2 + r[4] * i + r[5]
			# x2 = r[0] * i + r[1]
			if x2 < width and x2 > 0:
				r_funcy_points.append([x2, i])
		for i in array2y:
			x2 = r[0] * i**2 + r[1] * i**1 + r[2]
			# x2 = r[0] * i**5 + r[1] * i**4 + r[2] * i**3 + r[3] * i**2 + r[4] * i + r[5]
			# x2 = r[0] * i + r[1]
			r_funcy_xpred.append(x2)

		# r_points = np.array(r_funcx_points, dtype=np.int32)
		# cv2.polylines(im, [r_points], 0, (0,0,255))
		# r_points = np.array(r_funcy_points, dtype=np.int32)
		# cv2.polylines(im, [r_points], 0, (0,0,255))

		if mean_squared_error(array2y, r_funcx_ypred) < mean_squared_error(array2x, r_funcy_xpred):
			r_points = np.array(r_funcx_points, dtype=np.int32)
			cv2.polylines(im, [r_points], 0, (0,0,255))
		else:
			r_points = np.array(r_funcy_points, dtype=np.int32)
			cv2.polylines(im, [r_points], 0, (0,0,255))



	# print(len(l_points[0][0]))
	t1 = time.time()
	print(t1 - t0)



	# print(len(l_points[0][0]))



	cv2.imshow("", im)
	cv2.waitKey(0)
	# cv2.waitKey(0);
	# Press Q on keyboard to  exit 
	if cv2.waitKey(25) & 0xFF == ord('q'): 
	  break
   
  # Break the loop 
  else:  
	break
   
# When everything done, release  
# the video capture object 
cap.release() 
   
# Closes all the frames 
cv2.destroyAllWindows() 