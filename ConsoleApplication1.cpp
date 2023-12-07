#include <Windows.h>
#include <iostream>
#include <tuple>
#include <list>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <mutex>
#include <thread>
#include <random>
#include <functional>

using namespace std;

#pragma region "Comienzo de declaracion funciones y clases usadas"
//Explicacion: Clase encargada de calcular el fitness del individuo ni bien se "crea", y de mutarse a si mismo o cruzarse con otro
class Individuo {
public:
	Individuo();
	Individuo(vector<int>& individuo);
	int getFitness();
	vector<Individuo> cruzar(Individuo& x);
	string toString();
	bool operator<(Individuo& x);
	void mutar(bool mutar);
	Individuo clonar();
	string guardar();
	void cargar(string cadena);
private:
	void mutar1P();
	void mutarNP(int n);
	void mutarInsercion();
	Individuo cruceBasadoEnArcos(Individuo& x);
	vector<Individuo> crucePMX(Individuo& x);
	int elegirIdoneo(vector<vector<int>>& adyacencias, vector<int>& elecciones, int anterior);
	vector<vector<int>> getAdyacencias(Individuo& x);
	void calcularFitness();
	vector<int> valores;
	int fitness;
};

// Explicacion: Clase que se encarga de llevar cuenta de la poblacion y de mutar/cruzar/reemplazar a los individuos segun corresponda
class Generador {
public:
	Generador();
	Generador(int tamanioIndividuos, int tamanioPoblacion, bool generarPoblacion);
	void addIndividuo(Individuo& x);
	int getBestFitness();
	void iterar();
	vector<Individuo> getPoblacion();
	Individuo getIndividuo(int x);
	list<int> bestFitnessTiempo;
	list<int> fitnessTotalTiempo;
	int tamanioIndividuos, tamanioPoblacion, fitnessTotal, bestFitness;
private:
	list<Individuo> generarReemplazos(vector<Individuo>& padres);
	vector<Individuo> seleccionPadres();
	vector<Individuo> poblacion;
	void poblar();
};

void ejecutarPruebas();
void ejecutarPruebasFinas();
void ejecutarNormal(string arch = "", int corte = 0);
static int generarNroRandom(int rango, int offset);
vector<vector<int>> cargarArchivo(string archivo);
void cargarConfiguracion(string archivo = "");
void cargarPoblacionYConfiguracion(string archivo = "");
void guardarPoblacionYConfiguracion(string archivo = "");
#pragma endregion

enum OPERADORES_CRUCE { Basado_en_Arcos, PMX };
enum METODO_SELECCION_PADRES { Torneo, Ruleta_de_2 };
enum OPERADORES_MUTACION { Puntos, Insercion };
enum METODO_SELECCION_SUPERVIVIENTES { Reemplazo_N_Peores_Padres, N_Mejores_Padres };


static std::mt19937 random_number_engine;
static Generador generador;
static vector<vector<int>> matriz;
static std::mutex lockThreads, lockThreadsCruce;
static list<string> outputThreads = list<string>();
static list<string> outputPruebasFinas = list<string>();
static list<Individuo> outputCruza = list<Individuo>();
static bool pruebas = false;
static OPERADORES_CRUCE OPERADOR_CRUCE = Basado_en_Arcos;
static METODO_SELECCION_PADRES SELECCION_PADRES = Torneo;
static OPERADORES_MUTACION OPERADOR_MUTACION = Puntos;
static METODO_SELECCION_SUPERVIVIENTES SELECCION_SUPERVIVIENTES = Reemplazo_N_Peores_Padres;
static string archivoUsado;
static int RANGO_CIUDADES, TAMANIO_POBLACION, REEMPLAZO_GENERACIONAL, TAMANIO_TORNEOS, NRO_THREADS, PROB_MAX;
static int PROB_CRUCE, PROB_MUT_SIMPLE, PROB_MUT_COMPLEJA, CRUCES_MUTACION_COMPLEJA, ITERACIONES, LARGO_CRUCE_PMX;
double comienzoEjecucionT = 0, finEjecucionT = 0;


int main() {
	TAMANIO_POBLACION = 100;
	ITERACIONES = 10000;
	TAMANIO_TORNEOS = 5;
	PROB_MUT_SIMPLE = 3000;
	PROB_MUT_COMPLEJA = 10000;
	CRUCES_MUTACION_COMPLEJA = 4;
	PROB_CRUCE = 90000;
	LARGO_CRUCE_PMX = 6;
	REEMPLAZO_GENERACIONAL = 5;
	PROB_MAX = 100000;
	NRO_THREADS = 1;
	string opcion;
	cout << "Desea ejecutar las 32 pruebas automaticamente, las 625 pruebas finas o desea ejecutar algo en particular? (AUTO/FINO/PART)" << endl;
	cin >> opcion;
	if (opcion == "AUTO") ejecutarPruebas();
	else if ("FINO") ejecutarPruebasFinas();
	else ejecutarNormal();
	return 0;
}

#pragma region "Implementaciones Metodos"
#pragma region "Metodos Estaticos"
void probar(string arch, int prueba, int corte, bool fino) {
	if (fino) {
		PROB_CRUCE = 90000;
		PROB_MUT_SIMPLE = 3000;
		PROB_MUT_COMPLEJA = 10000;
		TAMANIO_TORNEOS = 5;
		PROB_CRUCE = PROB_CRUCE - ((prueba - 1) % 5) * 5000;
		PROB_MUT_SIMPLE = PROB_MUT_SIMPLE + (((prueba - 1) / 5) % 5) * 5000;
		PROB_MUT_COMPLEJA = PROB_MUT_COMPLEJA + (((prueba - 1) / 25) % 5) * 5000;
		TAMANIO_TORNEOS = TAMANIO_TORNEOS + (((prueba - 1) / 125) % 5);
	}
	if (archivoUsado != arch) {
		matriz = cargarArchivo(arch + ".atsp");
		RANGO_CIUDADES = matriz[0].size();
	}
	generador = Generador(RANGO_CIUDADES, TAMANIO_POBLACION, true);
	int i = 0, iters = 0, lastFitness = -1;
	bool x = false;
	HANDLE col = GetStdHandle(STD_OUTPUT_HANDLE);
	comienzoEjecucionT = (double)clock() / (double)CLOCKS_PER_SEC;
	while (i != ITERACIONES) {
		generador.iterar();
		i++;
		iters++;
		if (!fino && iters >= ITERACIONES / 10) {
			cout << "Mejor Fitness alcanzado: " << generador.getBestFitness() << "| Generacion actual: " << i << endl;
			iters = 0;
		}
		if (generador.getBestFitness() <= corte) break;
	}
	finEjecucionT = (double)clock() / (double)CLOCKS_PER_SEC;
	if (prueba < 10) guardarPoblacionYConfiguracion(arch + "-Prueba00" + to_string(prueba));
	else if (prueba < 100) guardarPoblacionYConfiguracion(arch + "-Prueba0" + to_string(prueba));
	else guardarPoblacionYConfiguracion(arch + "-Prueba" + to_string(prueba));
	string out = arch + "-Prueba";
	if (prueba < 10) out += "00";
	else if (prueba < 100) out += "0";
	out += to_string(prueba)
		+ "||TTotal: " + to_string(finEjecucionT - comienzoEjecucionT)
		+ "s||BestFit/Fit de corte: " + to_string(generador.getBestFitness()) + "/" + to_string(corte)
		+ "||FitPromedio: " + to_string(generador.fitnessTotal / TAMANIO_POBLACION)
		+ "||Iters/ItersMax: " + to_string(i) + "/" + to_string(ITERACIONES);
	if (fino) {
		string data = to_string(finEjecucionT-comienzoEjecucionT)+", "+to_string(generador.getBestFitness())+", "+to_string(generador.fitnessTotal/TAMANIO_POBLACION)+", "+to_string(i)+", ";
		data += to_string(PROB_CRUCE)+", "+to_string(PROB_MUT_SIMPLE)+", "+to_string(PROB_MUT_COMPLEJA)+", "+to_string(TAMANIO_TORNEOS) + ", ";
		data += to_string(OPERADOR_CRUCE)+", "+to_string(SELECCION_PADRES)+", "+to_string(OPERADOR_MUTACION)+", "+to_string(SELECCION_SUPERVIVIENTES);
		outputPruebasFinas.push_back(data);
		out += "||ProbCruce: " + to_string(PROB_CRUCE)
			+ "||ProbMut: " + to_string(PROB_MUT_SIMPLE)
			+ "||ProbMutCompleja: " + to_string(PROB_MUT_COMPLEJA)
			+ "||Torneos: " + to_string(TAMANIO_TORNEOS);
	} else {
		string data = to_string(SELECCION_PADRES)+ ", " 
			+ to_string(OPERADOR_MUTACION)+ ", " 
			+ to_string(SELECCION_SUPERVIVIENTES)+ ", "
			+ to_string(OPERADOR_CRUCE) + ", "
			+ to_string(finEjecucionT - comienzoEjecucionT) + ", " 
			+ to_string(generador.getBestFitness()) + ", " 
			+ to_string(generador.fitnessTotal / TAMANIO_POBLACION) + ", " 
			+ to_string(i);
		outputPruebasFinas.push_back(data);
		out += "||OPX: " + to_string(OPERADOR_CRUCE)
			+ "||SLP: " + to_string(SELECCION_PADRES)
			+ "||OPM: " + to_string(OPERADOR_MUTACION)
			+ "||SLS: " + to_string(SELECCION_SUPERVIVIENTES);
	}
	outputThreads.push_back(out);
	if (!fino) {
		system("cls");
		for (string s : outputThreads) {
			if (x)
				SetConsoleTextAttribute(col, 2);
			else
				SetConsoleTextAttribute(col, 6);
			cout << s << endl;
			x = !x;
		}
	}
}

void configurar() {
	string opcion;
	system("cls");
	cout << endl << "Cual desea que sea el tamanio de la poblacion? ";
	cin >> opcion;
	if (stoi(opcion) < 2) opcion = "100";
	TAMANIO_POBLACION = stoi(opcion);
	cout << endl << "Cual desea que sea la cantidad de iteraciones a ejecutar? ";
	cin >> opcion;
	if (stoi(opcion) < 1) opcion = "100";
	ITERACIONES = stoi(opcion);
	cout << endl << "Cual desea que sea el metodo de seleccion de padres? 0/1 (0 => Torneo, 1 => Ruleta de 2) ";
	cin >> opcion;
	if (stoi(opcion) > 2 || stoi(opcion) < 0) opcion = "0";
	SELECCION_PADRES = METODO_SELECCION_PADRES(stoi(opcion));
	if (stoi(opcion) == 0) {
		cout << endl << "Cual desea que sea el tamanio de los torneos? ";
		cin >> opcion;
		if (stoi(opcion) < 2 || stoi(opcion) > TAMANIO_POBLACION) opcion = "5";
		TAMANIO_TORNEOS = stoi(opcion);
	}
	cout << endl << "Cual desea que sea la probabilidad de mutacion? (el nro unicamente, ademas debe ser entero ejemplo 20 o 1 o 95 y multiplicado por 1000) ";
	cin >> opcion;
	if (stoi(opcion) < 1 || stoi(opcion) > PROB_MAX) opcion = "30";
	PROB_MUT_SIMPLE = stoi(opcion);
	cout << endl << "Cual desea que sea el operador de mutacion? 0/1 (0 => De 1 o N puntos, 1 => De insercion) ";
	cin >> opcion;
	if (stoi(opcion) > 2 || stoi(opcion) < 0) opcion = "0";
	OPERADOR_MUTACION = OPERADORES_MUTACION(stoi(opcion));
	if (stoi(opcion) == 0) {
		cout << endl << "Cual desea que sea la probabilidad de mutacion compleja? (el nro unicamente, ademas debe ser entero ejemplo 20 o 1 o 95 y multiplicado por 1000) ";
		cin >> opcion;
		if (stoi(opcion) < 1 || stoi(opcion) > PROB_MAX) opcion = "30";
		PROB_MUT_COMPLEJA = stoi(opcion);
		cout << endl << "Cual desea que sea la cantidad de cruces realizados por la mutacion compleja? ";
		cin >> opcion;
		CRUCES_MUTACION_COMPLEJA = stoi(opcion);
	}
	cout << endl << "Cual desea que sea la probabilidad de cruce? (el nro unicamente, ademas debe ser entero ejemplo 20 o 1 o 95 y multiplicado por 1000) ";
	cin >> opcion;
	if (stoi(opcion) < 1 || stoi(opcion) > PROB_MAX) opcion = "30";
	PROB_CRUCE = stoi(opcion);
	cout << endl << "Cual desea que sea el operador de cruce? 0/1 (0 => Basado en arcos, 1 => PMX) ";
	cin >> opcion;
	if (stoi(opcion) > 2 || stoi(opcion) < 0) opcion = "0";
	OPERADOR_CRUCE = OPERADORES_CRUCE(stoi(opcion));
	if (stoi(opcion) == 1) {
		cout << endl << "Cual desea que sea el largo del cruce PMX? ";
		cin >> opcion;
		if (stoi(opcion) >= matriz[0].size() || stoi(opcion) < 2) opcion = to_string(matriz[0].size() - 2);
		LARGO_CRUCE_PMX = stoi(opcion);
	}
	cout << endl << "Cual desea que sea el metodo de seleccion de supervivientes? 0/1 (0 => Reemplazo de N peores padres, 1 => N mejores padres quedan) ";
	cin >> opcion;
	if (stoi(opcion) > 2 || stoi(opcion) < 0) opcion = "0";
	SELECCION_SUPERVIVIENTES = METODO_SELECCION_SUPERVIVIENTES(stoi(opcion));
	cout << endl << "Cual desea que sea el reemplazo generacional? ";
	cin >> opcion;
	if (stoi(opcion) < 1 || stoi(opcion) > TAMANIO_POBLACION) opcion = "5";
	REEMPLAZO_GENERACIONAL = stoi(opcion);
	cout << endl << "Cuantos threads desea usar para el programa? ";
	cin >> opcion;
	NRO_THREADS = stoi(opcion);
}

void ejecutarPruebasFinas() {
	string arch = "p43";
	HANDLE col = GetStdHandle(STD_OUTPUT_HANDLE);
	NRO_THREADS = 16;
	OPERADOR_CRUCE = OPERADORES_CRUCE(0);
	SELECCION_PADRES = METODO_SELECCION_PADRES(0);
	OPERADOR_MUTACION = OPERADORES_MUTACION(0);
	SELECCION_SUPERVIVIENTES = METODO_SELECCION_SUPERVIVIENTES(1);
	for (int j = 0; j < 625; j++) {
		SetConsoleTextAttribute(col, 15);
		system("cls");
		cout << "Prueba: " << to_string(j + 1) << endl;
		probar(arch, (j + 1), 5620, true);
	}
	ofstream output("SalidaConsola.txt");
	for (string s : outputThreads)
		output << s << endl;
	output.close();
	SetConsoleTextAttribute(col, 15);
	ofstream outPt("DatosEjecucion.txt");
	for (string s : outputPruebasFinas)
		outPt << s << endl;
	outPt.close();
}

void ejecutarPruebas() {
	int opcion;
	cout << "Ingrese la cantidad de threads que desea utilizar para las pruebas..." << endl;
	cin >> NRO_THREADS;
	cout << "Desea analizar algun operador en especifico (0) o todos(1)?" << endl;
	cin >> opcion;
	int opcionCruce, opcionMutacion, opcionPadres, opcionSupervivientes, config;
	if (opcion == 0) {
		cout << "Si desea analizar algun operador de cruce en especifico seleccione 0 o 1 si elige 3 entonces se analizaran ambos" << endl;
		cin >> opcionCruce;
		cout << "Si desea analizar algun operador de mutacion en especifico seleccione 0 o 1 si elige 3 entonces se analizaran ambos" << endl;
		cin >> opcionMutacion;
		cout << "Si desea analizar algun operador de seleccion de padres en especifico seleccione 0 o 1 si elige 3 entonces se analizaran ambos" << endl;
		cin >> opcionPadres;
		cout << "Si desea analizar algun operador de seleccion de supervivientes en especifico seleccione 0 o 1 si elige 3 entonces se analizaran ambos" << endl;
		cin >> opcionSupervivientes;
	}
	cout << "Desea configurar las pruebas con ciertos valores (Para si 1, para no 0)?" << endl;
	cin >> config;
	if (config == 1)
		configurar();
	string arch = "br17";
	HANDLE col = GetStdHandle(STD_OUTPUT_HANDLE);
	for (int j = 0; j < 16; j++) {
		OPERADOR_CRUCE = OPERADORES_CRUCE(j % 2);
		int aux = j / 2;
		SELECCION_PADRES = METODO_SELECCION_PADRES(aux % 2);
		aux /= 2;
		OPERADOR_MUTACION = OPERADORES_MUTACION(aux % 2);
		aux /= 2;
		SELECCION_SUPERVIVIENTES = METODO_SELECCION_SUPERVIVIENTES(aux % 2);
		if (opcion == 0) {
			if (opcionCruce != 3 && opcionCruce != OPERADOR_CRUCE) continue;
			if (opcionMutacion != 3 && opcionMutacion != OPERADOR_MUTACION) continue;
			if (opcionPadres != 3 && opcionPadres != SELECCION_PADRES) continue;
			if (opcionMutacion != 3 && opcionMutacion != SELECCION_SUPERVIVIENTES) continue;
		}
		SetConsoleTextAttribute(col, 15);
		cout << "Comenzada prueba: " << to_string(j + 1) << " del archivo " << arch << endl;
		probar(arch, (j + 1), 39, false);
	}
	arch = "p43";
	for (int j = 0; j < 16; j++) {
		OPERADOR_CRUCE = OPERADORES_CRUCE(j % 2);
		int aux = j / 2;
		SELECCION_PADRES = METODO_SELECCION_PADRES(aux % 2);
		aux /= 2;
		OPERADOR_MUTACION = OPERADORES_MUTACION(aux % 2);
		aux /= 2;
		SELECCION_SUPERVIVIENTES = METODO_SELECCION_SUPERVIVIENTES(aux % 2);
		if (opcion == 0) {
			if (opcionCruce != 3 && opcionCruce != OPERADOR_CRUCE) continue;
			if (opcionMutacion != 3 && opcionMutacion != OPERADOR_MUTACION) continue;
			if (opcionPadres != 3 && opcionPadres != SELECCION_PADRES) continue;
			if (opcionMutacion != 3 && opcionMutacion != SELECCION_SUPERVIVIENTES) continue;
		}
		SetConsoleTextAttribute(col, 15);
		cout << "Comenzada prueba: " << to_string(j + 1) << " del archivo " << arch << endl;
		probar(arch, (j + 1), 5620, false);
	}
	ofstream output("SalidaConsola.txt");
	for (string s : outputThreads)
		output << s << endl;
	output.close();
	ofstream outPt("DatosEjecucion.txt");
	for (string s : outputPruebasFinas)
		outPt << s << endl;
	outPt.close();
	SetConsoleTextAttribute(col, 15);
}

void ejecutarNormal(string archivo, int corte) {
	system("cls");
	string opcion;
	cout << "Ingrese el nombre CON EXTENSION del archivo proveniente de la libreria TSPLIB95 que desea cargar" << endl;
	cout << "o sino ingrese Personalizado y podra elegir cargar una configuracion con o sin poblacion" << endl;
	cin >> opcion;
	if (opcion == "Personalizado") {
		system("cls");
		cout << "Selecciono para cargar personalizado" << endl;
		cout << "Desea cargar unicamente la configuracion de una poblacion previa(Config)" << endl;
		cout << "o tambien desea cargar la poblacion guardada(ConfigConPob)" << endl;
		cin >> opcion;
		if (opcion == "Config") cargarConfiguracion(archivo);
		else cargarPoblacionYConfiguracion();
	} else {
		system("cls");
		cout << "Selecciono para cargar el archivo " << opcion << endl;
		cout << "Desea cargar unicamente el archivo (Solo)" << endl;
		cout << "o desea tambien configurar los parametros (Config)?" << endl;
		matriz = cargarArchivo(opcion);
		cin >> opcion;
		if (opcion != "Solo") configurar();
		RANGO_CIUDADES = matriz[0].size();
		generador = Generador(RANGO_CIUDADES, TAMANIO_POBLACION, true);
	}
	cout << "Ingrese el minimo fitness el cual se debe alcanzar para cortar" << endl;
	cin >> corte;
	system("cls");
	int lastFitness = -1, iters = 0, i = 0;
	cout << "Comenzando ejecucion, se iran mostrando los nuevos mejores fitnesses conseguidos y la generacion en la cual se consiguieron" << endl;
	cout << "Cada 100 iteraciones consecutivas sin progreso, se mostrara un mensaje para mostrar que sigue ejecutando el programa" << endl;
	comienzoEjecucionT = (double)clock() / (double)CLOCKS_PER_SEC;
	while (i != ITERACIONES) {
		generador.iterar();
		i++;
		if (generador.getBestFitness() != lastFitness) {
			cout << "El Mejor Fitness de la poblacion ahora es " << generador.getBestFitness() << "| Generacion Actual: " << i << endl;
			lastFitness = generador.getBestFitness();
			iters = 0;
		} else {
			iters++;
			if (iters >= ITERACIONES / 100) {
				cout << "Generacion actual: " << i << endl;
				iters = 0;
			}
		}
		if (generador.getBestFitness() <= corte) break;
	}
	finEjecucionT = (double)clock() / (double)CLOCKS_PER_SEC;
	cout << "La ejecucion del algoritmo duro en total: " << finEjecucionT - comienzoEjecucionT << " segundos." << endl;
	for (Individuo i : generador.getPoblacion())
		if (i.getFitness() == generador.getBestFitness()) {
			cout << "La mejor solucion alcanzada fue: " << i.toString() << endl;
			break;
		}
	cout << "Iters/ItersMax: " << i << "/" << ITERACIONES << endl;
	do {
		cout << "Desea seguir la ejecucion del algoritmo por otras N iteraciones? Y/N" << endl;
		cin >> opcion;
		system("cls");
		if (opcion == "Y") {
			i = 0;
			while (i != ITERACIONES) {
				generador.iterar();
				i++;
				if (generador.getBestFitness() != lastFitness) {
					cout << "El Mejor Fitness de la poblacion ahora es " << generador.getBestFitness() << "| Generacion Actual: " << i << endl;
					lastFitness = generador.getBestFitness();
					iters = 0;
				} else {
					iters++;
					if (iters >= ITERACIONES / 100) {
						cout << "Generacion actual: " << i << endl;
						iters = 0;
					}
				}
				if (generador.getBestFitness() <= corte) break;
			}
		}
	} while (opcion == "Y");
	system("cls");
	cout << "Desea guardar la poblacion actual y sus parametros? Y/N" << endl;
	cin >> opcion;
	if (opcion == "Y") guardarPoblacionYConfiguracion(archivo);
}

static int generarNroRandom(int rango, int offset) {
	std::uniform_int_distribution<int> distribution(offset, rango + offset - 1);
	lockThreads.lock();
	int ret = distribution(random_number_engine);
	lockThreads.unlock();
	return ret;
}

vector<vector<int>> cargarArchivo(string archivo) {
	archivoUsado = archivo;
	vector<vector<int>> resultado = vector<vector<int>>();
	ifstream inputFile(archivo);
	if (!inputFile.good()) {
		cout << "Abortando el archivo no existe";
		Individuo x = generador.getPoblacion().at(TAMANIO_POBLACION + 1);
	}
	string exp;
	for (int i = 0; i < 7; i++) getline(inputFile, exp);
	while (true) {
		string line;
		getline(inputFile, line);
		if (line == "EOF") break;
		vector<int> fila = vector<int>();
		string word = "";
		for (char letter : line) {
			if (letter == ' ') {
				if (word != "") {
					fila.insert(fila.end(), stoi(word));
					word.clear();
				}
			} else
				word += letter;
		}
		if (word != "") fila.insert(fila.end(), stoi(word));
		resultado.insert(resultado.end(), fila);
	}
	inputFile.close();
	return resultado;
}

void cargarConfiguracion(string archivo) {
	string inpt;
	if (archivo == "") {
		cout << "Ingrese el nombre del archivo a cargar a continuacion (Se presupone que la extension es .txt)" << endl;
		cin >> inpt;
	} else inpt = archivo;
	ifstream inputFile(inpt + ".txt");
	if (!inputFile.good()) {
		cout << "Abortando el archivo no existe";
		Individuo x = generador.getPoblacion().at(1000);
	}
	getline(inputFile, inpt);
	archivoUsado = inpt.substr(inpt.find(':') + 2, inpt.size() - inpt.find(':') - 1);
	matriz = cargarArchivo(archivoUsado);
	getline(inputFile, inpt);
	TAMANIO_POBLACION = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	REEMPLAZO_GENERACIONAL = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	TAMANIO_TORNEOS = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	RANGO_CIUDADES = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	PROB_CRUCE = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	PROB_MUT_SIMPLE = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	PROB_MUT_COMPLEJA = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	CRUCES_MUTACION_COMPLEJA = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	ITERACIONES = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	LARGO_CRUCE_PMX = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	OPERADOR_CRUCE = OPERADORES_CRUCE(stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':'))));
	getline(inputFile, inpt);
	SELECCION_PADRES = METODO_SELECCION_PADRES(stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':'))));
	getline(inputFile, inpt);
	OPERADOR_MUTACION = OPERADORES_MUTACION(stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':'))));
	getline(inputFile, inpt);
	SELECCION_SUPERVIVIENTES = METODO_SELECCION_SUPERVIVIENTES(stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':'))));
	getline(inputFile, inpt);
	NRO_THREADS = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	generador = Generador(RANGO_CIUDADES, TAMANIO_POBLACION, true);
	inputFile.close();
}

void cargarPoblacionYConfiguracion(string archivo) {
	string inpt;
	if (archivo == "") {
		cout << "Ingrese el nombre del archivo a cargar a continuacion (Se presupone que la extension es .txt)" << endl;
		cin >> inpt;
	} else inpt = archivo;
	ifstream inputFile(inpt + ".txt");
	if (!inputFile.good()) {
		cout << "Abortando el archivo no existe";
		Individuo x = generador.getPoblacion().at(1000);
	}
	getline(inputFile, inpt);
	archivoUsado = inpt.substr(inpt.find(':') + 2, inpt.size() - inpt.find(':') - 1);
	matriz = cargarArchivo(archivoUsado);
	getline(inputFile, inpt);
	TAMANIO_POBLACION = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	REEMPLAZO_GENERACIONAL = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	TAMANIO_TORNEOS = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	RANGO_CIUDADES = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	PROB_CRUCE = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	PROB_MUT_SIMPLE = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	PROB_MUT_COMPLEJA = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	CRUCES_MUTACION_COMPLEJA = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	ITERACIONES = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	LARGO_CRUCE_PMX = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	getline(inputFile, inpt);
	OPERADOR_CRUCE = OPERADORES_CRUCE(stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':'))));
	getline(inputFile, inpt);
	SELECCION_PADRES = METODO_SELECCION_PADRES(stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':'))));
	getline(inputFile, inpt);
	OPERADOR_MUTACION = OPERADORES_MUTACION(stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':'))));
	getline(inputFile, inpt);
	SELECCION_SUPERVIVIENTES = METODO_SELECCION_SUPERVIVIENTES(stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':'))));
	getline(inputFile, inpt);
	NRO_THREADS = stoi(inpt.substr(inpt.find(':') + 1, inpt.size() - inpt.find(':')));
	generador = Generador(RANGO_CIUDADES, TAMANIO_POBLACION, false);
	for (int i = 0; i < TAMANIO_POBLACION; i++) {
		getline(inputFile, inpt);
		Individuo indi = Individuo();
		indi.cargar(inpt);
		generador.addIndividuo(indi);
	}
	inputFile.close();
}

void guardarPoblacionYConfiguracion(string archivo) {
	string exp;
	if (archivo == "") {
		cout << "Ingrese el nombre del archivo donde se guardara la poblacion y sus parametros (NO hace falta la extension, se guarda automaticamente como .txt)" << endl;
		cin >> exp;
	} else exp = archivo;
	ofstream outputFile(exp + ".txt");
	outputFile << "Archivo Usado: " << archivoUsado << "\nTamaño de la Poblacion: " << TAMANIO_POBLACION;
	outputFile << "\nReemplazo por Generacion: " << REEMPLAZO_GENERACIONAL << "\nTamaño de los Torneos: " << TAMANIO_TORNEOS;
	outputFile << "\nCantidad de ciudades: " << RANGO_CIUDADES;
	outputFile << "\nProbabilidad de cruce: " << PROB_CRUCE << "\nProbabilidad mutacion simple: " << PROB_MUT_SIMPLE;
	outputFile << "\nProbabilidad mutacion compleja: " << PROB_MUT_COMPLEJA << "\nCruces minimos de la mutacion compleja: " << CRUCES_MUTACION_COMPLEJA;
	outputFile << "\nIteraciones ejecutadas: " << ITERACIONES << "\nLargo del cruce PMX: " << LARGO_CRUCE_PMX;
	outputFile << "\nOperador de Cruce: " << OPERADOR_CRUCE << "\nMetodo seleccion de Padres: " << SELECCION_PADRES;
	outputFile << "\nOperador de Mutacion: " << OPERADOR_MUTACION << "\nMetodo seleccion de Supervivientes: " << SELECCION_SUPERVIVIENTES;
	outputFile << "\nCantidad de threads a usar: " << NRO_THREADS;
	Individuo mejorIndividuo;
	for (Individuo i : generador.getPoblacion()) {
		if (i.getFitness() == generador.getBestFitness()) mejorIndividuo = i;
		outputFile << endl << i.guardar();
	}
	outputFile << endl << "Tiempo de ejecucion (total): " << finEjecucionT - comienzoEjecucionT << endl;
	outputFile << "Best fitness a lo largo del tiempo: " << endl;
	for (int i : generador.bestFitnessTiempo) outputFile << i << ", ";
	outputFile << endl << "Fitness promedio a lo largo del tiempo: " << endl;
	for (int i : generador.fitnessTotalTiempo) outputFile << i << ", ";
	outputFile << endl << "Mejor solucion obtenida: " << mejorIndividuo.toString();
	outputFile.close();
}

static void cruzarPadresThreads(Individuo x, Individuo y) {
	vector<Individuo> resultados = x.cruzar(y);
	lockThreadsCruce.lock();
	while (resultados.size() != 0) {
		outputCruza.push_back(resultados.back());
		resultados.pop_back();
	}
	lockThreadsCruce.unlock();
}

#pragma endregion
#pragma region "Metodos Individuo"
// Verificado
Individuo Individuo::clonar() {
	return Individuo(valores);
}

// Verificado
string Individuo::guardar() {
	string cadena = "";
	for (int i : valores) cadena += to_string(i) + ", ";
	return cadena;
}

// Verificado
void Individuo::cargar(string cadena) {
	int ultimoIndice = 0, largo = 0, j = 0;
	for (int i = 0; i < RANGO_CIUDADES; i++) {
		bool listo = false;
		while (!listo && j < cadena.length()) {
			if (cadena[j] != ',') {
				j++;
				largo++;
			} else {
				int valor = stoi(cadena.substr(ultimoIndice, largo));
				valores.push_back(valor);
				j++;
				ultimoIndice = j;
				listo = true;
			}
		}
		if (j == cadena.length() && valores.size() != RANGO_CIUDADES) {
			cout << "Error al cargar individuo, la cantidad de ciudades no coincide abortando ejecucion." << endl;
			int x = valores.at(RANGO_CIUDADES);
			break;
		}
	}
	calcularFitness();
}

// Verificado
string Individuo::toString() {
	string cadena = "";
	for (int i : valores)
		cadena += to_string(i) + ", ";
	cadena += "|| Fitness: " + to_string(fitness) + ";";
	return cadena;
}

// Verificado
void Individuo::mutar1P() {
	int p1 = generarNroRandom(valores.size(), 0);
	int p2;
	do {
		p2 = generarNroRandom(valores.size(), 0);
	} while (p2 == p1);
	int aux = valores.at(p2);
	valores.at(p2) = valores.at(p1);
	valores.at(p1) = aux;
}

// Verificado
void Individuo::mutarNP(int n) {
	int i = 0;
	while (i < n) {
		int p1 = generarNroRandom(valores.size(), 0);
		int p2;
		do {
			p2 = generarNroRandom(valores.size(), 0);
		} while (p2 == p1);
		int aux = valores.at(p2);
		valores.at(p2) = valores.at(p1);
		valores.at(p1) = aux;
		i++;
	}
}

// Verificado
void Individuo::mutarInsercion() {
	int a = generarNroRandom(valores.size(), 0);
	int b, c;
	do {
		b = generarNroRandom(valores.size(), 0);
	} while (b == a);
	if (a == valores.size() - 1) {
		c = valores.at(0);
		valores.at(0) = valores.at(b);
		valores.at(b) = c;
	} else {
		c = valores.at(a + 1);
		valores.at(a + 1) = valores.at(b);
		valores.at(b) = c;
	}
}

// Verificado
void Individuo::mutar(bool mutar) {
	if (mutar || generarNroRandom(PROB_MAX, 0) >= (PROB_MAX - PROB_MUT_SIMPLE)) {
		if (OPERADOR_MUTACION == Puntos) {
			if (generarNroRandom(PROB_MAX, 0) > (PROB_MAX - PROB_MUT_COMPLEJA)) {
				int x = generarNroRandom(valores.size() % CRUCES_MUTACION_COMPLEJA, 2);
				mutarNP(x);
			} else {
				mutar1P();
			}
		} else {
			mutarInsercion();
		}
		calcularFitness();
	}
}

// Verificado
bool Individuo::operator<(Individuo& x) {
	return fitness < x.fitness;
}

// Verificado
Individuo::Individuo() {
	valores = vector<int>();
}

// Verificado
Individuo::Individuo(vector<int>& individuo) {
	valores = individuo;
	calcularFitness();
}

// Verificado
int Individuo::getFitness() {
	return fitness;
}

// Verificado
vector<vector<int>> Individuo::getAdyacencias(Individuo& x) {
	vector<vector<int>> adyacencias = vector<vector<int>>(RANGO_CIUDADES);
	int limite = RANGO_CIUDADES - 1;
	for (int i = 1; i < RANGO_CIUDADES; i++) {
		if (i == limite) {
			adyacencias.at(valores[i]).push_back(valores[0]);
			adyacencias.at(x.valores[i]).push_back(x.valores[0]);
		} else {
			adyacencias.at(valores[i]).push_back(valores[i + 1]);
			adyacencias.at(x.valores[i]).push_back(x.valores[i + 1]);
		}
		if (i == 0) {
			adyacencias.at(valores[i]).push_back(valores[limite]);
			adyacencias.at(x.valores[i]).push_back(x.valores[limite]);
		} else {
			adyacencias.at(valores[i]).push_back(valores[i - 1]);
			adyacencias.at(x.valores[i]).push_back(x.valores[i - 1]);
		}
	}
	return adyacencias;
}

// Verificado
int Individuo::elegirIdoneo(vector<vector<int>>& adyacencias, vector<int>& elecciones, int anterior) {
	//El idoneo seria aquel que tiene mas adyacencias
	int i = 0, previo = 0;
	int a = -1, b = -1, c = -1;
	for (int j : adyacencias.at(anterior)) {
		if (a == -1) {
			previo = j;
			a = j;
			continue;
		} else if (b == -1) {
			previo = j;
			b = j;
			continue;
		} else if (c == -1) {
			previo = j;
			c = j;
			continue;
		} else {
			previo = j;
			continue;
		}
		if (a == j) {
			previo = j;
			break;
		} else if (b == j) {
			previo = j;
			break;
		} else if (c == j) {
			previo = j;
			break;
		}
	}
	//vector<int> cantidades = vector<int>(RANGO_CIUDADES);
	////Contamos cuantas adyacencias tiene cada valor
	//for (int j : adyacencias.at(anterior)) cantidades.at(j)++;
	////Elegimos aquel indice que tenga mas adyacencias
	//for (int j = 0; j < cantidades.size(); j++) {
	//	if (cantidades.at(j) > cantidades.at(previo))
	//		previo = j;
	//	else if (cantidades.at(j) == cantidades.at(previo))
	//		previo = (adyacencias.at(previo).size() >= adyacencias.at(j).size()) ? previo : j;
	//}
	////Obtenemos el valor que tiene mas adyacencias
	while (previo != elecciones.at(i)) i++;
	return i;
}

// Verificado
Individuo Individuo::cruceBasadoEnArcos(Individuo& x) {
	vector<int> resultado = vector<int>();
	// Obtenemos la tabla con las adyacencias
	vector<vector<int>> adyacencias = getAdyacencias(x);
	// Inicializamos la lista de resultados y valores disponibles
	vector<int> elecciones = vector<int>();
	for (int i = 0; i < RANGO_CIUDADES; i++) elecciones.push_back(i);
	bool disponible = false;
	int posicionNominado = generarNroRandom(RANGO_CIUDADES, 0);
	// Mientras todavia tenemos elecciones
	while (elecciones.size() != 0) {
		// Obtenemos el individuo
		int nominado = elecciones.at(posicionNominado);
		// Guardamos la eleccion en la solucion y ademas la borramos de las posibles elecciones siguientes
		elecciones.erase(elecciones.begin() + posicionNominado);
		resultado.push_back(nominado);
		int borrados = 0;
		// Borramos adyacencias en otros nodos posibles
		for (int i = 0; i < RANGO_CIUDADES; i++) {
			bool checked = false;
			int j = 0;
			if (borrados == 4) break;
			if (adyacencias.at(i).size() == 0) continue;
			while (!checked) {
				if (adyacencias.at(i).at(j) == nominado) {
					adyacencias.at(i).erase(adyacencias.at(i).begin() + j);
					borrados++;
				} else
					j++;
				if (j == adyacencias.at(i).size()) checked = true;
			}
		}
		// Nos fijamos si esta eleccion tiene alguna adyacencia posible
		disponible = (adyacencias.at(nominado).size() != 0) ? true : false;
		if (elecciones.size() != 0) {
			if (!disponible) {
				// Si no hay un valor posible adyacente al anterior elegimos uno random
				posicionNominado = generarNroRandom(elecciones.size(), 0);
			} else {
				// Si tenemos algun adyacente entonces nos fijamos de obtener el idoneo
				posicionNominado = elegirIdoneo(adyacencias, elecciones, resultado.back());
			}
		}
	}
	return resultado;
}

// Verificado
vector<Individuo> Individuo::crucePMX(Individuo& x) {
	//Inicializamos variables
	vector<Individuo> resultados = vector<Individuo>();
	vector<int> indA = vector<int>(valores.size()), indB = vector<int>(valores.size());
	vector<tuple<int, int>> reemplazosA = vector<tuple<int, int>>(), reemplazosB = vector<tuple<int, int>>();
	//Obtenemos el inicio de donde vamos a intercambiar
	int inicio = generarNroRandom(valores.size() - LARGO_CRUCE_PMX - 1, 0);
	//Hacemos el shuffle
	for (int i = 0; i < LARGO_CRUCE_PMX; i++) {
		int a = valores.at(inicio + i), b = x.valores.at(inicio + i);
		indA.at(inicio + i) = b;
		indB.at(inicio + i) = a;
		reemplazosA.push_back(tuple<int, int>(b, a));
		reemplazosB.push_back(tuple<int, int>(a, b));
	}
	//Completamos el resto de valores sin agregar
	for (int i = 0; i < valores.size(); i++) {
		//Evitamos tocar los valores que ya fueron intercambiados
		if (i < inicio || i >= inicio + LARGO_CRUCE_PMX) {
			indA.at(i) = valores.at(i);
			indB.at(i) = x.valores.at(i);
			bool repetir = false;
			//Hasta que no se detecte ningun conflicto (intentar agregar algun valor que seria repetido) en indA
			do {
				repetir = false;
				//Buscamos por que valor reemplazarlo
				for (int j = 0; j < LARGO_CRUCE_PMX; j++)
					//Si lo encontramos lo reemplazamos y marcamos para seguir buscando
					if (indA.at(i) == get<0>(reemplazosA.at(j)) && indA.at(i) != get<1>(reemplazosA.at(j))) {
						indA.at(i) = get<1>(reemplazosA.at(j));
						repetir = true;
					}
			} while (repetir);
			//Hasta que no se detecte ningun conflicto (intentar agregar algun valor que seria repetido) en indB
			do {
				repetir = false;
				//Buscamos por que valor reemplazarlo
				for (int j = 0; j < LARGO_CRUCE_PMX; j++)
					//Si lo encontramos lo reemplazamos y marcamos para seguir buscando
					if (indB.at(i) == get<0>(reemplazosB.at(j)) && indB.at(i) != get<1>(reemplazosB.at(j))) {
						indB.at(i) = get<1>(reemplazosB.at(j));
						repetir = true;
					}
			} while (repetir);
		}
	}
	resultados.push_back(Individuo(indA));
	resultados.push_back(Individuo(indB));
	return resultados;
}

// Verificado
vector<Individuo> Individuo::cruzar(Individuo& x) {
	vector<Individuo> resultado = vector<Individuo>();
	if (OPERADOR_CRUCE == Basado_en_Arcos) {
		resultado.push_back(cruceBasadoEnArcos(x));
	} else {
		resultado = crucePMX(x);
	}
	for (int i = 0; i < resultado.size(); i++)
		resultado.at(i).mutar(false);
	return resultado;
}

// Verificado
void Individuo::calcularFitness() {
	int nuevoFitness = 0;
	for (int i = 0; i < valores.size() - 1; i++) {
		nuevoFitness += matriz[valores[i]][valores[i + 1]];
	}
	nuevoFitness += matriz[valores.back()][valores.front()];
	fitness = nuevoFitness;
}
#pragma endregion
#pragma region "Metodos Generador"
// Verificado
Generador::Generador() {
	tamanioPoblacion = TAMANIO_POBLACION;
	tamanioIndividuos = RANGO_CIUDADES;
	bestFitness = 0;
	fitnessTotal = 0;
}

// Verificado
Generador::Generador(int tamInd, int tamPob, bool genPob) {
	tamanioIndividuos = tamInd;
	tamanioPoblacion = tamPob;
	bestFitness = 0;
	fitnessTotal = 0;
	fitnessTotalTiempo.clear();
	bestFitnessTiempo.clear();
	if (genPob) poblar();
}

// Verificado
Individuo Generador::getIndividuo(int x) {
	return poblacion.at(x);
}

// Verificado
void Generador::addIndividuo(Individuo& x) {
	if (bestFitness == -1 || bestFitness > x.getFitness()) bestFitness = x.getFitness();
	fitnessTotal += x.getFitness();
	poblacion.push_back(x);
}

// Verificado
vector<Individuo> Generador::getPoblacion() {
	return poblacion;
}

// Verificado
int Generador::getBestFitness() {
	return bestFitness;
}

// Verificado
void Generador::iterar() {
	vector<Individuo> padres = seleccionPadres();
	list<Individuo> reemplazos = generarReemplazos(padres);
	list<Individuo> poblacionOrdenada = list<Individuo>();
	for (Individuo i : poblacion) poblacionOrdenada.push_back(i);
	poblacionOrdenada.sort();
	reemplazos.sort();
	while (reemplazos.size() > TAMANIO_POBLACION) reemplazos.pop_back();
	if (SELECCION_SUPERVIVIENTES == Reemplazo_N_Peores_Padres) {
		for (int i = 0; i < REEMPLAZO_GENERACIONAL; i++) {
			poblacionOrdenada.pop_back();
			poblacionOrdenada.push_front(reemplazos.front());
			reemplazos.pop_front();
		}
	} else {
		for (int i = 0; i < REEMPLAZO_GENERACIONAL; i++) {
			reemplazos.pop_back();
			reemplazos.push_front(poblacionOrdenada.front());
			poblacionOrdenada.pop_front();
		}
		poblacionOrdenada = reemplazos;
	}
	poblacion.clear();
	fitnessTotal = 0;
	bestFitness = -1;
	for (Individuo i : poblacionOrdenada) {
		poblacion.push_back(i);
		fitnessTotal += i.getFitness();
		if (bestFitness == -1 || bestFitness > i.getFitness())
			bestFitness = i.getFitness();
	}
	bestFitnessTiempo.push_back(bestFitness);
	fitnessTotalTiempo.push_back(fitnessTotal / TAMANIO_POBLACION);
}

// Verificado
vector<Individuo> Generador::seleccionPadres() {
	vector<Individuo> padres = vector<Individuo>();
	while (padres.size() < TAMANIO_POBLACION) {
		if (SELECCION_PADRES == Torneo) {
			vector<Individuo> contrincantes = vector<Individuo>();
			while (contrincantes.size() < TAMANIO_TORNEOS) contrincantes.push_back(getIndividuo(generarNroRandom(TAMANIO_POBLACION, 0)));
			while (contrincantes.size() > 1)
				if (contrincantes.at(0).getFitness() > contrincantes.at(1).getFitness()) {
					contrincantes.erase(contrincantes.begin());
				} else {
					contrincantes.erase(contrincantes.begin() + 1);
				}
			padres.push_back(contrincantes.at(0));
		} else {
			list<Individuo> valores = list<Individuo>();
			for (Individuo i : poblacion) valores.push_back(i);
			valores.sort();
			vector<tuple<Individuo, double>> z = vector<tuple<Individuo, double>>();
			std::list<Individuo>::iterator it_Comienzo = valores.begin(), it_Final = valores.end();
			double fit = 0;
			for (int i = 0; i < TAMANIO_POBLACION; i++) {
				it_Final--;
				fit += (double)it_Final->getFitness() / (double)fitnessTotal;
				z.push_back(tuple<Individuo, double>((*it_Comienzo), fit));
				it_Comienzo++;
			}
			//Se genera un nro de 0 a 1000 para generar una probabilidad, digamos se genera el 985, la prob resultante seria de 98,5
			while (padres.size() < TAMANIO_POBLACION) {
				double elegido = (double)generarNroRandom(1000, 0) / 1000.0;
				int it_A = 0, it_B = TAMANIO_POBLACION - 1;
				bool encontradoA = false, encontradoB = false;
				while (!(encontradoA && encontradoB)) {
					if (get<1>(z.at(it_A)) >= elegido)
						encontradoA = true;
					if (get<1>(z.at(it_B)) <= 1 - elegido)
						encontradoB = true;
					if (!encontradoA && it_A < TAMANIO_POBLACION)
						it_A++;
					else
						encontradoA = true;
					if (!encontradoB && it_B > 0)
						it_B--;
					else
						encontradoB = true;
				}
				if (it_A == it_B) {
					if (it_A == 0)
						it_B++;
					else
						it_B--;
				}
				padres.push_back(get<0>(z.at(it_A)));
				padres.push_back(get<0>(z.at(it_B)));
			}
		}
	}
	return padres;
}

// Verificado
list<Individuo> Generador::generarReemplazos(vector<Individuo>& padres) {
	list<Individuo> reemplazos;
	std::vector<thread> threads = vector<thread>(NRO_THREADS);
	int threadsUsados = 0;
	while (TAMANIO_POBLACION >= reemplazos.size()) {
		int x = generarNroRandom(TAMANIO_POBLACION, 0);
		Individuo padre = padres.at(x);
		x = generarNroRandom(TAMANIO_POBLACION, 0);
		if (generarNroRandom(PROB_MAX, 0) > (PROB_MAX - PROB_CRUCE)) {
			Individuo padreB = padres.at(x);
			threads[threadsUsados] = thread(cruzarPadresThreads, padre, padreB);
			threadsUsados++;
			if (threadsUsados == NRO_THREADS) {
				for (auto& th : threads) if (th.joinable()) th.join();
				threadsUsados = 0;
				for (Individuo i : outputCruza) reemplazos.push_back(i);
				outputCruza.clear();
			}
		} else {
			Individuo hijo = padre.clonar();
			hijo.mutar(true);
			reemplazos.push_back(hijo);
			hijo = padres.at(x).clonar();
			hijo.mutar(true);
			reemplazos.push_back(hijo);
		}
	}
	if (threadsUsados != 0) {
		for (auto& th : threads)
			if (th.joinable())
				th.join();
		outputCruza.clear();
	}
	return reemplazos;
}

// Verificado
void Generador::poblar() {
	fitnessTotal = 0;
	bestFitness = -1;
	poblacion = vector<Individuo>();
	vector<int> ejemplo = vector<int>(), copiaEjemplo = vector<int>(), aux;
	for (int i = 0; i < RANGO_CIUDADES; i++) ejemplo.push_back(i);
	for (int i = 0; i < TAMANIO_POBLACION; i++) {
		vector<int> individuo = vector<int>();
		while (ejemplo.size() != 0) {
			int x = generarNroRandom(ejemplo.size(), 0);
			int z = ejemplo.at(x);
			ejemplo.erase(ejemplo.begin() + x);
			individuo.push_back(z);
			copiaEjemplo.push_back(z);
		}
		aux = ejemplo;
		ejemplo = copiaEjemplo;
		copiaEjemplo = aux;
		Individuo ejemplar = Individuo(individuo);
		fitnessTotal += ejemplar.getFitness();
		if (bestFitness == -1 || bestFitness > ejemplar.getFitness()) bestFitness = ejemplar.getFitness();
		poblacion.push_back(ejemplar);
	}
	bestFitnessTiempo.push_back(bestFitness);
}
#pragma endregion
#pragma endregion