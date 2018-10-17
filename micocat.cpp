//send ttl udp
#include "opencv2/opencv.hpp"
#include <iostream>
#include <string>
#include <sys/time.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/socket.h>
#include<pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
using namespace cv;
using namespace std;
using namespace dnn;

#define MICRO_IN_SEC 1000000.00

double microtime();
int ssd();
int voice_cat();
int voice_hello();
int send_cat(float x, float y);
int main(int argc, char** argv)
{
	voice_hello();
	ssd();
	return 0;
}
double microtime() {

	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return tv.tv_sec + tv.tv_usec / MICRO_IN_SEC;
}
int ssd()
{
	double start_time, dt, dt_err;
	start_time = microtime();
	dt_err = microtime() - start_time;
	double voice_time = microtime();
	String prototxt = "MobileNetSSD_deploy.prototxt";
	String caffemodel = "MobileNetSSD_deploy.caffemodel";
	Net net = readNetFromCaffe(prototxt, caffemodel);

	const char* classNames[] = { "background", "aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat", "chair",
		"cow", "diningtable", "dog", "horse", "motorbike", "person", "pottedplant", "sheep", "sofa", "train", "tvmonitor" };

	float detect_thresh = 0.25;
	if (true)
	{
		net.setPreferableTarget(0);
	}

	VideoCapture capture(0);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	Mat image;
	int picN = 0;
	char filename[256];
	int max_lock = 10;
	Point cat_point[10];
	int cat_area[10];
	float cat_x, cat_y;
	while (1)
	{
		start_time = microtime();
		int trigger = 0;
		capture >> image;
		net.setInput(blobFromImage(image, 1.0 / 127.5, Size(300, 300), Scalar(127.5, 127.5, 127.5), true, false));
		Mat cvOut = net.forward();
		Mat detectionMat(cvOut.size[2], cvOut.size[3], CV_32F, cvOut.ptr<float>());
		for (int i = 0; i < detectionMat.rows; i++)
		{
			int obj_class = detectionMat.at<float>(i, 1);
			float confidence = detectionMat.at<float>(i, 2);

			if (confidence > detect_thresh)
			{
				size_t objectClass = (size_t)(detectionMat.at<float>(i, 1));

				int xLeftBottom = static_cast<int>(detectionMat.at<float>(i, 3) * image.cols);
				int yLeftBottom = static_cast<int>(detectionMat.at<float>(i, 4) * image.rows);
				int xRightTop = static_cast<int>(detectionMat.at<float>(i, 5) * image.cols);
				int yRightTop = static_cast<int>(detectionMat.at<float>(i, 6) * image.rows);

				ostringstream ss;
				int tmpI = 100 * confidence;
				ss << tmpI;
				String conf(ss.str());

				Rect object((int)xLeftBottom, (int)yLeftBottom,
					(int)(xRightTop - xLeftBottom),
					(int)(yRightTop - yLeftBottom));
				if (classNames[objectClass] == "cat" || classNames[objectClass] == "dog")
				{
					trigger++;
					rectangle(image, object, Scalar(0, 0, 255), 1);
					String label = String(classNames[objectClass]) + ": " + conf + "%";
					putText(image, label, Point(xLeftBottom, yLeftBottom + 30 * (i + 1)), 2, 0.8, Scalar(0, 0, 255), 2);
					if (trigger < max_lock)
					{
						cat_point[trigger - 1].x = object.x + cvRound(object.width / 2.0);
						cat_point[trigger - 1].y = object.y + cvRound(object.height / 2.0);
						cat_area[trigger - 1] = object.area();
					}
				}
				else if (classNames[objectClass] == "pottedplant" || classNames[objectClass] == "sofa")
				{
					rectangle(image, object, Scalar(0, 255, 0), 1);
					String label = String(classNames[objectClass]) + ": " + conf + "%";
					putText(image, label, Point(xLeftBottom, yLeftBottom + 30 * (i + 1)), 2, 0.7, Scalar(0, 255, 0), 2);
				}
				else if (classNames[objectClass] == "person" || classNames[objectClass] == "bicycle")
				{
					rectangle(image, object, Scalar(255, 255, 0), 1);
					String label = String(classNames[objectClass]) + ": " + conf + "%";
					putText(image, label, Point(xLeftBottom, yLeftBottom + 30 * (i + 1)), 2, 0.7, Scalar(255, 255, 0), 2);
				}
				else
				{
					rectangle(image, object, Scalar(255, 0, 0), 1);
					String label = String(classNames[objectClass]) + ": " + conf + "%";
					putText(image, label, Point(xLeftBottom, yLeftBottom + 30 * (i + 1)), 2, 0.7, Scalar(255, 0, 0), 2);
				}

			}
		}
		if (trigger>=1)
		{
			sprintf(filename, "/home/pi/test/detec/%06d.jpg",picN);
			picN++;
			if (picN > 10000)
				picN = 0;
			imwrite(filename, image);
			cout << "Waning" << endl;

			int cat_n = 0;
			int cat_area_tmp = 0;
			Point bigcat_point;
			for (cat_n = 0; cat_n < trigger; cat_n++)
			{
				if (cat_area[cat_n] > cat_area_tmp)
				{
					cat_area_tmp = cat_area[cat_n];
					bigcat_point = cat_point[cat_n];
				}
			}
			cat_x = (bigcat_point.x - 320)*0.075;
			cat_y = (240 - bigcat_point.y)*0.075;
			cout << "location: x = " << cat_x << "	y = " << cat_y << endl;
			if (microtime() - voice_time > 10)
			{
				voice_hello();
				send_cat(cat_x,cat_y);
				voice_time = microtime();
			}
		}
		dt = microtime() - start_time - dt_err;
		cout << "Cost time: " << dt << " s" << endl;
		imshow("test", image);
		waitKey(1);
	}
	return 0;
}
int voice_hello()
{
	int fd = -1;
	//fd = open("/dev/ttySAC2", O_RDWR | O_NOCTTY | O_NDELAY);
	fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		perror("Open Serial Port Error!\n");
		return -1;
	}
	struct termios options;
	tcgetattr(fd, &options);
	//115200, 8N1
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);
	write(fd, "", 1);
	close(fd);
	return 0;
}
int voice_cat()
{
	int fd = -1;
	//fd = open("/dev/ttySAC2", O_RDWR | O_NOCTTY | O_NDELAY);
	fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		perror("Open Serial Port Error!\n");
		return -1;
	}
	struct termios options;
	tcgetattr(fd, &options);
	//115200, 8N1
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);
	write(fd, "AT+TTS=cat\r", 12);
	close(fd);
	cout << "voice" << endl;
	return 0;
}
int send_cat(float x, float y)
{
	float *floatBuff = new float[2];//
	floatBuff[0] = x;
	floatBuff[1] = y;
	char *buff=(char *)floatBuff;//
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6000);
	//   addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	//  bind(sock, (struct sockaddr *)&addr, sizeof(addr)); 
	sendto(sock, buff, strlen(buff)+1, 0, (struct sockaddr *)&addr, sizeof(addr));
	delete floatBuff;
	close(sock);

}