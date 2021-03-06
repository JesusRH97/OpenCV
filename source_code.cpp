// Proyecto_base.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//
#include"pch.h"
#include <iostream>
#include <vector>
#include "opencv\cv.hpp"
#include <fstream> //Libreria para trabajar con ficheros
#define ESCAPE 27

using namespace std;
using namespace cv;

/*Función para obtener los puntos de las esquinas del guión del trabajo*/
vector <Point3f> GenerateCorners();
/*Función para obtener las esquinas detectadas*/
vector <Point2f> ObtainCorners(vector<Point> puntos);

vector <Point3f> GenerateCorners3D();

vector <Point3f> GenerateTop3D();

Mat img_src;
int eleccion = 0;

int main(int argc, char** argv)
{	

	/*Estas líneas de código están dedicadas a leer los datos de los coeficientes de distorsión
	y de la cámara que importamos de un fichero de texto*/
	double camera_matrix1, camera_matrix2, camera_matrix3, camera_matrix4, camera_matrix5, 
		camera_matrix6, camera_matrix7, camera_matrix8, camera_matrix9;
	double distorsion1, distorsion2, distorsion3, distorsion4, distorsion5;
	ifstream eDatos(argv[1]); //Se abre el fichero de propietarios

	if (eDatos.is_open())
	{ //Se comprueba que el fichero se ha abierto correctamente
		eDatos >> camera_matrix1 >> camera_matrix2 >> camera_matrix3 >> camera_matrix4 >> camera_matrix5 >> camera_matrix6 >> camera_matrix7 >> camera_matrix8 >> camera_matrix9; //el primer dato del archivo es el numero de propietarios que contiene
		eDatos >> distorsion1 >> distorsion2 >> distorsion3 >> distorsion4 >> distorsion5;

	}
	else
	{
		std::cout << "Error al cargar los datos" << std::endl;
		std::cout << "" << std::endl;
	}
	eDatos.close(); //Se cierra el fichero

	double datos_camera_matrix[9] = { camera_matrix1, camera_matrix2, camera_matrix3, camera_matrix4, camera_matrix5, 
		camera_matrix6, camera_matrix7, camera_matrix8, camera_matrix9 };
	double datos_coeficiente_distorsion[5] = { distorsion1, distorsion2, distorsion3, distorsion4, distorsion5 };
	Mat camera_matrix = Mat(3, 3, CV_64FC1, &datos_camera_matrix);
	Mat coeficientes_distorsion = Mat(5, 1, CV_64FC1, &datos_coeficiente_distorsion);

	/*Objetos para tratar la imagen y dibujar el cubo y la pirámide*/
	Mat img_grey, filtered_img(img_src.size(), img_src.type());
	Mat rvec, tvec;


	/*Vector donde inicialmente guardamos las esquinas detectadas*/
	vector<vector<Point>> contours0;

	/*Vector de puntos 3D de las esquinas que sale en el guión del trabajo*/
	vector <Point3f> corners;
	/*Vector de puntos 3D de los cuatro vértices restantes del cubo*/
	vector<Point3f> corners3D;
	/*Vector que almacena los vértices de la pirámide*/
	vector<Point3f> piramide;

	/*Inicializamos los tres vectores anteriores con las siguientes funciones comentadas más abajo*/ 
	corners = GenerateCorners();
	corners3D = GenerateCorners3D();
	piramide = GenerateTop3D();
	
	/*Vector de puntos convertidos a tipo Point2f.
	Este vector l inicializaremos más adelante, cuando usemos la función "contours" 
	para detectar las esquinas del Aruco*/
	vector<Point2f> esquinas;

	vector<Vec4i> hierarchy;
	Mat curva;
	unsigned int lados = 4;

	/*Variables necesarias para la umbralización*/
	long hist_vector[256];
	int threshold_value;
	int low_threshold = 50;
	int high_threshold = 2 * low_threshold;
	int kernel_size = 3;

	/*Abrimos el vídeo sobre el que vamos a trabajar*/
	VideoCapture capture;
	//capture.open(argv[2]);
	capture.open(0);

	char pressedKey = 0;

	bool success;
	bool detectado = false;

	/*Variables y objetos necesarios para declarar un objeto de tipo VideoWriter*/
	int frame_width = static_cast<int>(capture.get(CAP_PROP_FRAME_WIDTH)); //Ancho del vídeo de salida
	int frame_height = static_cast<int>(capture.get(CAP_PROP_FRAME_HEIGHT)); //Alto del vídeo de salida
	Size frame_size(frame_width, frame_height);
	int frames_per_second = 30;

	/*Creamos el objeto de tipo VideoWriter para guardar los fotogramas que tomemos del vídeo
	que importamos*/
	VideoWriter oVideoWriter("resultado.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), frames_per_second, frame_size, true);

	if (!capture.isOpened()) {
		cout << "Error loading the image";
	}

	else {
		

		while (1) {

			success = capture.read(img_src);

			if (success == false) {
				cout << "Cannot read the frame from stream";
				return 1;
			}
			
			
			/*Creamos una matriz para almacenar la imagen umbralizada*/
			Mat im_thresholded(img_src.size(), CV_8UC1);
			/*Filtramos la imagen para reducir el ruido*/
			medianBlur(img_src, img_src, 5);
			threshold_value = 100;
			/*La pasamos a blanco y negro para umbralizarla luego*/
			cvtColor(img_src,img_grey,CV_RGB2GRAY);
			/*La umbralizamos*/
			threshold(img_grey, im_thresholded, threshold_value, 255, THRESH_BINARY);
			
			/*Canny: 2:1 3x3*/
			Canny(im_thresholded, filtered_img, low_threshold, high_threshold, kernel_size);
			
			/*Encontrar contornos en la imagen ya umbralizada*/
			findContours(filtered_img, contours0, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

			/*Buscamos contornos. Para ello, creamos una serie de matrices donde vamos a guardar los
			todos los contornos que encontremos*/
			/*Mat contours(filtered_img.size(), CV_8UC3, Scalar(0, 0, 0));
			Mat contourImage(filtered_img.size(), CV_8UC3, Scalar(0, 0, 0));*/
			/*Con esta función buscamos todos los contornos de la imagen procesada*/
			//contours.resize(contours0.size());
			/*Una vez obtenidos todos los contornos, pasamos a buscar primero, cuales están formados
			por cuatro puntos*/
			for (int k = 0; k < contours0.size(); k++)
			{
				/*ApproxPolyDP: vamos a ver si el contorno detectado se aproxima a una forma
				poligonal cerrada formada por cuatro puntos*/
				approxPolyDP(contours0[k], contours0[k], 0.12 * arcLength(contours0[k], 1), true);

				if ((unsigned int) contours0[k].size()==lados)
					{
					
					/*De todos los contornos de cuatro puntos detectados, vamos a intentar filtrar algunos
					que puedan no ser el Aruco simplemente calculando el área que encierran estos cuatro puntos*/
					if (contourArea(contours0[k], 1) > 3000)
					{

						if (waitKey(30) == 97) 
						{
							eleccion++;
						}

						/*Le pasamos el vector con las esquinas*/
						/*"Esquinas" es el vector de las esquinas del Aruco en 2D (Point 2f)
						que vamos a utilizar tanto para dibujar el cubo como la pirámide, 
						porque ambas comparten la base*/
						esquinas = ObtainCorners(contours0[k]);
						/*Pintamos la base del Aruco solamente*/
						drawContours(img_src, contours0, k, Scalar(57, 255, 20), 5);
					

						/*Este if se encarga del algoritmo de dibujar el cub0*/
						if (eleccion % 2 == 0) {
							/*SolvePnp da como output las matrices "rvec" y "tvec", que son las matrices de traslación y rotación
							con respecto a la cámara. De esta manera, podemos convertir de forma muy fácil puntos en 3D a
							puntos 2D en la pantalla del ordenador*/
							/*"corners" es el mismo vector que esquinas pero en 3D (Point 3f)*/
							solvePnP(corners, esquinas, camera_matrix, coeficientes_distorsion, rvec, tvec);
							/*Obtenemos los puntos 3D del vector corners3D a puntos 2D en el vector esquinas para así
							luego poder pintar de forma fácil las líneas que unen los puntos*/
							projectPoints(corners3D, rvec, tvec, camera_matrix, coeficientes_distorsion, esquinas);
							/*Empezamos uniendo cada punto de la base con su correspondiente del vector esquinas*/
							line(img_src, esquinas[0], contours0[k][0], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[1], contours0[k][1], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[2], contours0[k][2], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[3], contours0[k][3], Scalar(57, 255, 20), 3);
							/*Unimos los puntos del vector esquinas entre sí*/
							line(img_src, esquinas[0], esquinas[1], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[1], esquinas[2], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[2], esquinas[3], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[3], esquinas[0], Scalar(57, 255, 20), 3);
						}

						else {
							/*Pintamos la pirámide*/
							/*Da como output las matrices "rvec" y "tvec", que son las matrices de traslación y rotación
							con respecto a la cámara. De esta manera, podemos convertir de forma muy fácil puntos en 3D a
							puntos 2D en la pantalla del ordenador*/
							/*"corners" es el mismo vector que esquinas pero en 3D (Point 3f)*/
							solvePnP(corners, esquinas, camera_matrix, coeficientes_distorsion, rvec, tvec);
							/*Obtenemos los puntos 3D del vector corners3D a puntos 2D en el vector esquinas para así
							luego poder pintar de forma fácil las líneas que unen los puntos*/
							projectPoints(piramide, rvec, tvec, camera_matrix, coeficientes_distorsion, esquinas);
							/*Unimos las esquinas del Aruco con el punto más alto de la pirámide*/
							line(img_src, esquinas[0], contours0[k][0], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[1], contours0[k][1], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[1], esquinas[0], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[0], esquinas[2], Scalar(57, 255, 20), 3);
							line(img_src, esquinas[1], esquinas[2], Scalar(57, 255, 20), 3);
							line(img_src, contours0[k][0], esquinas[2], Scalar(57, 255, 20), 3);
							line(img_src, contours0[k][1], esquinas[2], Scalar(57, 255, 20), 3);
							
						}

					}

			    }
				
			}

			/*Escribimos en el vídeo para guardarlo*/
			oVideoWriter.write(img_src);

			//Creamos el texto de los fotogramas por segundo
			float fps=capture.get(CV_CAP_PROP_FPS);
			string fps2 = to_string(fps);
			string fps3 = fps2 + "fps";
			putText(img_src, fps3, Point(5, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(125, 12, 145), 2);

			//Create window canvas to show image
			namedWindow("original", CV_WINDOW_AUTOSIZE);

			//Show image in the name of the window
			imshow("original", img_src);
		
		    waitKey(1);
			
		}

	
		/*Liberamos memoria*/
		img_src.release();
		destroyWindow("original");
		return 0;
		capture.release();
	}
}

/*Función que le pasamos el vector de contornos que hemos detectado del Aruco.
Básicamente, está función convierte estos puntos a otros de tipo "Point2f", 
porque si miramos la bibliografía de OpenCV uno de los vectores de entrada
que hay que meterle a SolvePnp es uno de tipo Point2f*/
vector <Point2f> ObtainCorners(vector<Point> puntos) {
	vector<Point2f> points;
	
	for (int i = 0; i < puntos.size(); i++) {
		points.push_back(puntos[i]);
		cout << points[i]<<endl;
	}

	return points;
}

/*Función para obtener las esquinas del Aruco pero en 3D*/
vector <Point3f> GenerateCorners() {

	vector <Point3f> puntos;

	float x, y, z;

	x = -0.1 / 2;
	y = 0.1 / 2;
	z = 0;

	puntos.push_back(Point3f(x, y, z));

	x = 0.1 / 2;
	y = 0.1 / 2;
	z = 0;

	puntos.push_back(Point3f(x, y, z));

	x = 0.1 / 2;
	y = -0.1 / 2;
	z = 0;

	puntos.push_back(Point3f(x, y, z));

	x = -0.1 / 2;
	y = -0.1 / 2;
	z = 0;

	puntos.push_back(Point3f(x, y, z));

	return puntos;
}

/*Función para obtener los otros cuatro vértices restantes del cubo*/
vector <Point3f> GenerateCorners3D() {

	vector <Point3f> puntos;

	float x, y, z;

	x = -0.1 / 2;
	y = 0.1 / 2;
	z = 0.1;

	puntos.push_back(Point3f(x, y, z));

	x = 0.1 / 2;
	y = 0.1 / 2;
	z = 0.1;

	puntos.push_back(Point3f(x, y, z));

	x = 0.1 / 2;
	y = -0.1 / 2;
	z = 0.1;

	puntos.push_back(Point3f(x, y, z));

	x = -0.1 / 2;
	y = -0.1 / 2;
	z = 0.1;

	puntos.push_back(Point3f(x, y, z));




	return puntos;
}

/*Función para generar la pirámide*/
vector <Point3f> GenerateTop3D() {

	vector <Point3f> puntos;

	float x, y, z;

	x = -0.05;
	y = 0.05;
	z = 0.1;

	puntos.push_back(Point3f(x, y, z));

	x = 0.05;
	y = 0.05;
	z = 0.1;

	puntos.push_back(Point3f(x, y, z));

	x = 0.00;
	y = -0.05;
	z = 0.05;

	puntos.push_back(Point3f(x, y, z));

	return puntos;

}


// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

// Sugerencias para primeros pasos: 1. Use la ventana del Explorador de soluciones para agregar y administrar archivos
//   2. Use la ventana de Team Explorer para conectar con el control de código fuente
//   3. Use la ventana de salida para ver la salida de compilación y otros mensajes
//   4. Use la ventana Lista de errores para ver los errores
//   5. Vaya a Proyecto > Agregar nuevo elemento para crear nuevos archivos de código, o a Proyecto > Agregar elemento existente para agregar archivos de código existentes al proyecto
//   6. En el futuro, para volver a abrir este proyecto, vaya a Archivo > Abrir > Proyecto y seleccione el archivo .sln
