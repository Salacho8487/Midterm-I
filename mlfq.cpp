// Santiago Salazar Gil
// Parcial 1 Practico / Implementacion Algoritmos MFLQ C++
// Sistemas Operativos

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <iomanip>
using namespace std;

class Proceso {
public:
    string id;       
    int burstTime;   
    int arrivalTime; 
    int queueLevel;  
    int priority;     // Prioridad (5 > 1)

    int completionTime = 0;
    int turnaroundTime = 0;
    int waitingTime = 0;
    int responseTime = -1;

    int remainingTime;      // Tiempo restante para ejecutar
    bool started = false;   // Marca si ya comenzó a ejecutarse

    Proceso(string _id, int bt, int at, int q, int pr)
        : id(_id), burstTime(bt), arrivalTime(at), queueLevel(q), priority(pr) {
        remainingTime = bt;
    }
};

class MLFQ {
private:
    vector<Proceso> procesos;     // Lista de todos los procesos   
    vector<string> ganttChart; 

public:
    MLFQ(const vector<Proceso>& listaProcesos) {
        procesos = listaProcesos;
    }

    void ejecutar(const vector<string>& config, const string& outputFile) {
        int tiempo = 0;
        int procesosCompletados = 0;
        int n = procesos.size();

        // Ordenar procesos por tiempo de llegada
        sort(procesos.begin(), procesos.end(),
            [](const Proceso& a, const Proceso& b) { return a.arrivalTime < b.arrivalTime; });

        // Bucle principal hasta que todos los procesos terminen
        while (procesosCompletados < n) {
            bool ejecutado = false;

            // Revisar colas en orden jerárquico
            for (int nivel = 0; nivel < (int)config.size(); nivel++) {
                string politica = config[nivel];
                vector<int> listos;

                // Seleccionar procesos listos para este nivel
                for (int i = 0; i < n; i++) {
                    if (procesos[i].arrivalTime <= tiempo && procesos[i].remainingTime > 0 && procesos[i].queueLevel == nivel + 1) {
                        listos.push_back(i);
                    }
                }

                if (listos.empty()) continue;

                int idx = -1;


                // Round Robin
                if (politica.find("RR") == 0) {
                    int quantum = stoi(politica.substr(3, politica.size() - 4));
                    idx = listos[0]; 
                    Proceso& p = procesos[idx];

                    // Si nunca había empezado, registramos Response Time
                    if (!p.started) {
                        p.responseTime = tiempo;
                        p.started = true;
                    }

                    int ejecutar = min(quantum, p.remainingTime);
                    for (int t = 0; t < ejecutar; t++) {
                        ganttChart.push_back(p.id);
                        tiempo++;
                    }
                    p.remainingTime -= ejecutar;

                    if (p.remainingTime == 0) {
                        p.completionTime = tiempo;
                        p.turnaroundTime = p.completionTime - p.arrivalTime;
                        p.waitingTime = p.turnaroundTime - p.burstTime;
                        procesosCompletados++;
                    }
                    ejecutado = true;
                }

                // SJF
                else if (politica == "SJF") {
                    // Elegimos el proceso con menor BT
                    idx = *min_element(listos.begin(), listos.end(), [&](int a, int b) {
                        return procesos[a].burstTime < procesos[b].burstTime;
                    });
                    Proceso& p = procesos[idx];

                    if (!p.started) {
                        p.responseTime = tiempo;
                        p.started = true;
                    }

                    for (int t = 0; t < p.remainingTime; t++) {
                        ganttChart.push_back(p.id);
                        tiempo++;
                    }
                    p.remainingTime = 0;
                    p.completionTime = tiempo;
                    p.turnaroundTime = p.completionTime - p.arrivalTime;
                    p.waitingTime = p.turnaroundTime - p.burstTime;
                    procesosCompletados++;
                    ejecutado = true;
                }

                // STCF
                else if (politica == "STCF") {
                    // Elegimos el proceso con menor remainingTime
                    idx = *min_element(listos.begin(), listos.end(), [&](int a, int b) {
                        return procesos[a].remainingTime < procesos[b].remainingTime;
                    });
                    Proceso& p = procesos[idx];

                    if (!p.started) {
                        p.responseTime = tiempo;
                        p.started = true;
                    }

                    ganttChart.push_back(p.id);
                    tiempo++;
                    p.remainingTime--;

                    if (p.remainingTime == 0) {
                        p.completionTime = tiempo;
                        p.turnaroundTime = p.completionTime - p.arrivalTime;
                        p.waitingTime = p.turnaroundTime - p.burstTime;
                        procesosCompletados++;
                    }
                    ejecutado = true;
                }

                if (ejecutado) break; 
            }

            if (!ejecutado) {
                tiempo++;       // Si nadie se ejecutó, avanzamos el tiempo
            }
        }

        guardarResultados(outputFile);
    }

    // Guardado en archivo de salida
    void guardarResultados(const string& archivoSalida) {
        ofstream out(archivoSalida);
        double totalWT = 0, totalCT = 0, totalRT = 0, totalTAT = 0;

        out << left << setw(8) << "ID" << setw(6) << "BT" << setw(6) << "AT"
            << setw(6) << "Q" << setw(6) << "Pr" << setw(6) << "WT"
            << setw(6) << "CT" << setw(6) << "RT" << setw(6) << "TAT" << "\n";

        for (auto& p : procesos) {
            out << left << setw(8) << p.id
                << setw(6) << p.burstTime
                << setw(6) << p.arrivalTime
                << setw(6) << p.queueLevel
                << setw(6) << p.priority
                << setw(6) << p.waitingTime
                << setw(6) << p.completionTime
                << setw(6) << p.responseTime
                << setw(6) << p.turnaroundTime << "\n";

            totalWT += p.waitingTime;
            totalCT += p.completionTime;
            totalRT += p.responseTime;
            totalTAT += p.turnaroundTime;
        }

        // Promedios de las metricas
        int n = procesos.size();
        out << "WT=" << totalWT / n
            << "; CT=" << totalCT / n
            << "; RT=" << totalRT / n
            << "; TAT=" << totalTAT / n << ";\n";
    }
};

class Simulador {
public:
    static vector<Proceso> leerProcesos(const string& filename) {
        ifstream archivo(filename);
        if (!archivo) {
            cerr << "Error al abrir el archivo de entrada.\n";
            exit(1);
        }

        vector<Proceso> listaProcesos;
        string linea;

        getline(archivo, linea); 

        while (getline(archivo, linea)) {
            if (linea.empty()) continue;
            stringstream ss(linea);
            string id;
            int bt, at, q, pr;

            // Lectura imput
            ss >> id >> bt >> at >> q >> pr;
            listaProcesos.emplace_back(id, bt, at, q, pr);
        }

        return listaProcesos;
    }
};

int main() {
    vector<Proceso> listaProcesos = Simulador::leerProcesos("mlfq3.txt");

    vector<vector<string>> configuraciones = {
        {"RR(1)", "RR(3)", "RR(4)", "SJF"},
        {"RR(2)", "RR(3)", "RR(4)", "STCF"},
        {"RR(3)", "RR(5)", "RR(6)", "RR(20)"}
    };

    for (int i = 0; i < (int)configuraciones.size(); i++) {
        string outputFile = "mlfq_output_" + to_string(i + 1) + ".txt";
        MLFQ scheduler(listaProcesos);
        scheduler.ejecutar(configuraciones[i], outputFile);
        cout << "Resultados guardados en " << outputFile << endl;
    }

    return 0;
}