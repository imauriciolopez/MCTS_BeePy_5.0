#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <chrono>
#define INF (unsigned)!((int)0)

using namespace std;

//estructura para los nodos del arbol
struct node{
    //estructuras de navegación
    node* pad;                   //apuntador al nodo padre
    vector<node*> hijos;         //vector de apuntadores a los nodos hijos
    //data contenida:
    vector<vector<int> > tablero;//tablero del juego específico en el nodo
    int g_p;                     //dice si el tablero contenido es ganador(1), perdedor(-1), o sigue el juego(0)
    double uct;                  //calificación asignada al tablero
    int n;                       //cantidad de veces que se ha visitado el nodo
    int wins;                    //cantidad de victorias del subarbol del que este nodo es raiz
    int turno;                   //turno del siguiente tiro
    
    //el constructor pide:
    //vector<vector<int> > tablero_, int turno_,int g_p_
    node(vector<vector<int> > tablero_, int turno_,int g_p_):
                                                tablero(tablero_), 
                                                g_p(g_p_),
                                                turno(turno_),
                                                uct(6.0), 
                                                n(0),
                                                wins(0),
                                                pad(NULL), 
                                                hijos(){}
};

//clase del hexapawn, aquí se encuentran todas las reglas del juego
class hexapawn{
private:
    unsigned seed;
    mt19937 gen;
    int _;//? esto no se usa
    random_device rd;
    uniform_int_distribution<int> dist;
    default_random_engine e;
public:
    hexapawn():seed(chrono::system_clock::now().time_since_epoch().count()), gen(seed), dist(1, 1000){} // también podrías omitir dist si no lo usas{}

    //Recibe el tamaño del tablero y da el tablero puesto junto con el turno 1
    //regresa una tupla, de tablero y turno
    tuple<vector<vector<int> >, int> poner_el_juego(int n, int turno=0){
        /*
        pone el juego desde el inicio, donde el tablero es siempre cuadrado, y la cantidad
        de piezas dependen del tamaño del tablero
        Entrada:
        int n: tamaño del tablero
        int turno=0: turno actual (preset a cero, o sea turno humano)
        
        Salida:
        salida: descripción de la salida
        */
        //declaraciones
        int i, j;
        vector<vector<int> >tablero;
        vector<int> fila;
        //iteramos sobre todas las posiciones del tablero
        for(i=0;i<n;i++){
            for(j=0;j<n;j++){
                if(!i){
                    fila.push_back(2);
                }
                else if(i!=n-1){
                    fila.push_back(0);
                }
                else{
                    fila.push_back(1);
                }
            }
            tablero.push_back(fila);
            fila.resize(0);
        }
        return make_tuple(tablero, (turno==0)?(1):((turno==1)?(1):((turno==2)?(2):(1))));
    }
    
    //Imprime de manera bonita el estado del juego, junto con el turno
    void ver_tablero(vector<vector<int> >tablero, int turno=1){
        int i, j, k, count=0;
        cout<<"╔";
        for(i=0;i<tablero.size()*4-1;i++){
            cout<<"═";
            count++;
            if(count==100){
                exit(5);
            }
        }
        //cout<<"╗   filas     turno de: "<<turno;
        if(turno==1){
            cout<<"╗   filas     turno de: X";
        }
        else{
            cout<<"╗   filas     turno de: O";
        }
        cout<<endl;
        for(i=0;i<tablero.size();i++){
            cout<<"║";
            for(j=0;j<tablero[1].size();j++){
                //HERE
                if(tablero[i][j]==0){
                    cout<<"   ";
                }
                if(tablero[i][j]==1){
                    cout<<" X ";
                }
                if(tablero[i][j]==2){
                    cout<<" O ";
                }
                if(j!=tablero[1].size()-1){
                    cout<<"│";
                }
            }
            cout<<"║ ";
            cout<<i<<endl;
            if(i!=tablero.size()-1){
                cout<<"║";
                for(j=0;j<tablero[1].size();j++){
                    for(k=0;k<3;k++){
                        cout<<"─";
                    }
                    if(j!=tablero[1].size()-1){
                        cout<<"┼";
                    }
                }
                cout<<"║";
                cout<<endl;
            }
        }
        cout<<"╚";
        for(i=0;i<tablero[1].size()*4-1;i++){
            cout<<"═";
        }
        cout<<"╝\n ";
        for(i=0;i<tablero[1].size();i++){
            cout<<" "<<i<<"  ";
        }
        cout<<"\n\ncolumnas\n\n\n";
    }
    
    //Cambia de turno para el siguiente
    int siguiente_turno(int turno){
        return (turno==1)?(2):(1);
    }
    
    //filas original, columnas original, filas terminal, columnas terminal
    //te dice si el tiro que quieres hacer es legal o no
    bool tiro_legal(int f_o, int c_o, int f_t, int c_t, vector<vector<int> >tablero){
        if(f_t<0||f_t>tablero.size()-1||c_t<0||c_t>tablero.size()-1){
            return false;
        }
        if(tablero[f_o][c_o]==1){
            return (((f_t==f_o-1)&&(c_o==c_t))&&(tablero[f_t][c_t]==0))||(((f_t==f_o-1)&&(c_o==c_t-1))&&(tablero[f_t][c_t]==2))||(((f_t==f_o-1)&&(c_o==c_t+1))&&(tablero[f_t][c_t]==2));
        }
        else if(tablero[f_o][c_o]==2){
            return (((f_t==f_o+1)&&(c_o==c_t))&&(tablero[f_t][c_t]==0))||(((f_t==f_o+1)&&(c_o==c_t-1))&&(tablero[f_t][c_t]==1))||(((f_t==f_o+1)&&(c_o==c_t+1))&&(tablero[f_t][c_t]==1));
        }
        else{
            return false;
        }
    }
    
    //recibe un tablero junto con el turno y da un tiro aleatorio, regresa el tablero con el
    //tiro regristrado y el siguiente turno en una tupla
    tuple<vector<vector<int> >, int> tiro_random(vector<vector<int> >tablero, int turno){
        //posibles almacena un vector, de coordenadas de todas las fichas del jugador del que queremos hacer el tiro
        vector<tuple<int, int> > posibles;
        vector<int> direcciones;
        tuple<int, int> elegido;
        int i, j;
        for(i=0;i<tablero.size();i++){
            for(j=0;j<tablero[0].size();j++){
                if(tablero[i][j]==turno){
                    posibles.push_back(make_tuple(i,j));
                }
            }
        }
        shuffle(posibles.begin(), posibles.end(), gen);
        while(!posibles.empty()){
            direcciones={-1,0,1};
            elegido=posibles[posibles.size()-1];
            shuffle(direcciones.begin(), direcciones.end(), gen);
            while(!direcciones.empty()){
                //cout  <<  get<0>(elegido)  <<  get<1>(elegido)  <<  ((turno==1)?(get<0>(elegido)-1):(get<0>(elegido)+1))  <<  get<1>(elegido)+direcciones[direcciones.size()-1]<<"\n";
                if(tiro_legal(get<0>(elegido),
                get<1>(elegido),
                (turno==1)?(get<0>(elegido)-1):(get<0>(elegido)+1),
                get<1>(elegido)+direcciones[direcciones.size()-1],
                tablero)){
                    tablero[get<0>(elegido)][get<1>(elegido)]=0;
                    tablero[(turno==1)?(get<0>(elegido)-1):(get<0>(elegido)+1)][get<1>(elegido)+direcciones[direcciones.size()-1]]=turno;
                    return make_tuple(tablero,(turno==1)?(2):(1));
                }
                else{
                    direcciones.pop_back();
                }
            }
            posibles.pop_back();
        }
        return make_tuple(tablero, 0);
    }
    
    tuple< vector<vector<vector<int> > >,int> todos_los_tiros(vector<vector<int> >tablero, int turno){
        vector<tuple<int, int> > posibles;
        vector<int> direcciones;
        tuple<int, int> elegido;
        int i, j;

        vector<vector<vector<int> > > todos;
        vector<vector<int> > tablero_copia=tablero;

        for(i=0;i<tablero.size();i++){
            for(j=0;j<tablero[0].size();j++){
                if(tablero[i][j]==turno){
                    posibles.push_back(make_tuple(i,j));
                }
            }
        }
        shuffle(posibles.begin(), posibles.end(), gen);
        while(!posibles.empty()){
            direcciones={-1,0,1};
            elegido=posibles[posibles.size()-1];
            shuffle(direcciones.begin(), direcciones.end(), gen);
            while(!direcciones.empty()){
                //cout  <<  get<0>(elegido)  <<  get<1>(elegido)  <<  ((turno==1)?(get<0>(elegido)-1):(get<0>(elegido)+1))  <<  get<1>(elegido)+direcciones[direcciones.size()-1]<<"\n";
                if(tiro_legal(get<0>(elegido),
                get<1>(elegido),
                (turno==1)?(get<0>(elegido)-1):(get<0>(elegido)+1),
                get<1>(elegido)+direcciones[direcciones.size()-1],
                tablero)){
                    tablero[get<0>(elegido)][get<1>(elegido)]=0;
                    tablero[(turno==1)?(get<0>(elegido)-1):(get<0>(elegido)+1)][get<1>(elegido)+direcciones[direcciones.size()-1]]=turno;
                    todos.push_back(tablero);
                    tablero=tablero_copia;
                    direcciones.pop_back();
                }
                else{
                    direcciones.pop_back();
                }
            }
            posibles.pop_back();
        }
        return make_tuple(todos, turno);
    }

    //condicion 1 de ganar: llegar al otro lado, regresa -1 si nadie satisface esta 
    //condición, de lo contrario, regresa la ficha ganadora
    bool ganar_1(vector<vector<int> >tablero){
        int i, j;
        for(i=0;i<tablero.size();i+=tablero.size()-1){
            for(j=0;j<tablero[0].size();j++){
                if(!i){
                    if(tablero[i][j]==1){
                        return true;
                    }
                }
                else{
                    if(tablero[i][j]==2){
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    //condicion 2 de ganar: uno de los dos ya no tiene fichas, regresa -1 si nadie satisface, 
    //de lo contrario regresa la ficha ganadora
    
    //creo que aquí hay un problema, no siempre detecta cuando ya no pueden seguir los tiros
    //al menos creo que esa es la razón de un problema que surgió al generar los tiros aleatorios
    //que para terminar los tiros pide un tablero con finalizacion, pero no se detectó por parte de esta funcion
    //pero ese fue un solo caso particular, en los demás si sigue funcionando
    int ganar_2(vector<vector<int> >tablero){
        vector<int> conteo;
        conteo.push_back(0);
        conteo.push_back(0);
        int i,j;
        for(i=0;i<tablero.size();i++){
            for(j=0;j<tablero[0].size();j++){
                if(tablero[i][j]){
                    conteo[tablero[i][j]-1]++;
                }
            }
        }
        return (conteo[0]==0)?(true):(conteo[1]==0);
    }
    
    //condicion 3 de ganar: ya nadie puede tirar. regresa 1 si esta condicion se cumple,
    //regresa 0 de lo contrario
    bool ganar_3(vector<vector<int> >tablero){
        vector<tuple<int, int> > fichas;
        vector<int> direcciones;
        int i, j;
        for(i=0;i<tablero.size();i++){
            for(j=0;j<tablero[0].size();j++){
                if(tablero[i][j]){
                    fichas.push_back(make_tuple(i,j));
                }
            }
        }
        for(i=0;i<fichas.size();i++){
            direcciones={-1,0,1};
            for(j=0;j<3;j++){
                if(tiro_legal(get<0>(fichas[i]),get<1>(fichas[i]),(tablero[get<0>(fichas[i])][get<1>(fichas[i])]==1)?(get<0>(fichas[i])-1):(get<0>(fichas[i])+1),get<1>(fichas[i])+direcciones[j],tablero)){
                    return false;
                }
            }
        }
        return true;
    }
    
    //verifica las condiciones de terminado del juego
    bool condiciones_de_terminado(vector<vector<int> >tablero){
        return ganar_1(tablero)||ganar_2(tablero)||ganar_3(tablero);
    }

    //le das un tablero y turno, y termina el juego con tiros aleatorios. Regresa 
    //la ficha ganadora. Adicionalmente, puedes ver qué onda con el juego
    int terminar_juego(vector<vector<int> >tablero, int turno, bool verbose=0){
        tuple<vector<vector<int> >, int> instancia;
        int n=0;
        while(1){
            instancia=tiro_random(tablero, turno);
            tablero=get<0>(instancia);
            turno=get<1>(instancia);
            if(verbose){
                ver_tablero(tablero,turno);
            }
            if(condiciones_de_terminado(tablero)||turno==0){
                if(verbose){
                    if(turno==1){
                        cout<<"VICTORIA DE O"<<endl;
                    }
                    else{
                        cout<<"VICTORIA DE X"<<endl;
                    }
                }
                return (turno==1)?(2):(1);
            }
        }
        return 0;
    }

    //te da un juego contra humano dado un tamaño de tablero, no regresa nada, solo es
    //para entretenimiento
    void juego_random_contra_humano(int tamano){
        cout<<"JUEGO CONTRA HUMANO:"<<endl;
        tuple<vector<vector<int> >, int> inicio=poner_el_juego(tamano);
        vector<vector<int> > tablero=get<0>(inicio);
        int turno=get<1>(inicio);
        int seguir=1, f_o, c_o, f_t, c_t;
        ver_tablero(tablero,turno);
        while(seguir){
            while(1){
                cout<<"\nFilas(origen):";
                cin>>f_o;
                cout<<"\nColumnas(origen):";
                cin>>c_o;
                cout<<"\nFilas(terminal):";
                cin>>f_t;
                cout<<"\nColumnas(terminal):";
                cin>>c_t;
                if(tiro_legal(f_o,c_o,f_t,c_t,tablero)){
                    tablero[f_o][c_o]=0;
                    tablero[f_t][c_t]=1;
                    break;
                }
                else{
                    cout<<"\nTiro no permitido, intenta de nuevo.";
                }
            }
            if(turno==1){
                turno=2;
            }
            else{
                turno=1;
            }
            ver_tablero(tablero,turno);
            if(condiciones_de_terminado(tablero)){
                cout<<"\nVICTORIA DE:";
                if(turno==1){
                    cout<<"O\n\n\nQuieres seguir?:(1=si, 0=no)";
                }
                else{
                    cout<<"X\n\n\nQuieres seguir?:(1=si, 0=no)";
                }
                cin>>seguir;
                if(seguir){
                    inicio=poner_el_juego(tamano);
                    tablero=get<0>(inicio);
                    turno=get<1>(inicio);
                    ver_tablero(tablero,turno);
                }
                else{
                    if(turno==1){
                        cout<<"\nGracias por jugar, suerte para la próxima! :)";
                    }
                    else{
                        cout<<"\nGracias por jugar, mejoraré para ganarte la próxima! >:)";
                    }
                }
            }
            else{
                cout<<"\ntiro de maquina:\n";
                inicio=tiro_random(tablero,turno);
                tablero=get<0>(inicio);
                turno=get<1>(inicio);
                ver_tablero(tablero,turno);
                if(condiciones_de_terminado(tablero)){
                    cout<<"\nVICTORIA DE:";
                    if(turno==1){
                        cout<<"O\n\n\nQuieres seguir?:(1=si, 0=no)";
                    }
                    else{
                        cout<<"X\n\n\nQuieres seguir?:(1=si, 0=no)";
                    }
                    cin>>seguir;
                    if(seguir){
                        inicio=poner_el_juego(tamano);
                        tablero=get<0>(inicio);
                        turno=get<1>(inicio);
                        ver_tablero(tablero,turno);
                    }
                    else{
                        if(turno==1){
                            cout<<"\nGracias por jugar, suerte para la próxima! :)";
                        }
                        else{
                            cout<<"\nGracias por jugar, mejoraré para ganarte la próxima! >:)";
                        }
                    }
                }
            }
        }
    }
    
    

    ~hexapawn()=default;
};

//clase que contiene los método del arbol
class tree{
    private:
        unsigned seed;
        mt19937 gen;
        int _;
        random_device rd;
        uniform_int_distribution<int> dist;
        uniform_real_distribution<> dist_;
        default_random_engine e;
    public:
        node *raiz;                   //atributo
        hexapawn hexa;
        tree(node* root):raiz(root), seed(chrono::system_clock::now().time_since_epoch().count()),gen(seed), dist(1, 1000), dist_(0.0, 1.0){} // también podrías omitir dist si no lo usas{}

        node* nuevo_nodo(vector<vector<int> > tablero_, int turno_,int g_p_){
            return new node(tablero_, turno_, g_p_);
        }
        
        node* insertar(node* padr, vector<vector<int> > tablero_, int turno_,int g_p_){
            if(padr!=NULL){
                node *hijo=new node(tablero_, turno_, g_p_);
                hijo->pad=padr;
                padr->hijos.push_back(hijo);
                return hijo;
            }
            else{
                return NULL;
            }
        }
 
        //RECORRIDOS
        void postorden(node* actual){
            if(actual!=NULL){
                int i, j;
                for(i=0;i<actual->hijos.size();i++){
                    postorden(actual->hijos[i]);
                }
                cout<<endl;
                for(i=0;i<actual->tablero.size();i++){
                    for(j=0;j<actual->tablero[0].size();j++){
                        if(actual->tablero[i][j]==0){
                            cout<<" ";
                        }
                        else if(actual->tablero[i][j]==1){
                            cout<<"X";
                        }
                        else{
                            cout<<"O";
                        }
                    }
                    cout<<endl;
                }
                cout<<"turno:"<<actual->turno<<"g_p:"<<actual->turno<<endl;
            }
        }
        
        void preorden(node* actual){
            if(actual!=NULL){
                int i, j;
                for(i=0;i<actual->hijos.size();i++){
                    postorden(actual->hijos[i]);
                }
                cout<<endl;
                for(i=0;i<actual->tablero.size();i++){
                    for(j=0;j<actual->tablero[0].size();j++){
                        if(actual->tablero[i][j]==0){
                            cout<<" ";
                        }
                        else if(actual->tablero[i][j]==1){
                            cout<<"X";
                        }
                        else{
                            cout<<"O";
                        }
                    }
                    cout<<endl;
                }
                cout<<"turno:"<<actual->turno<<"g_p:"<<actual->turno<<endl;
                for(i=0;i<actual->hijos.size();i++){
                    preorden(actual->hijos[i]);
                }
            }
        }

        void fancy_print(node* actual, int indentacion=0){
            if(actual!=NULL){
                int i, j,k;
                for(i=0;i<actual->hijos.size();i++){
                    fancy_print(actual->hijos[i],indentacion+7);
                }
                for(i=0;i<actual->tablero.size();i++){
                    for (k=0;k<indentacion;k++){
                        cout << " ";
                    }
                    for(j=0;j<actual->tablero[0].size();j++){
                        if(actual->tablero[i][j]==0){
                            cout<<" ";
                        }
                        else if(actual->tablero[i][j]==1){
                            cout<<"X";
                        }
                        else{
                            cout<<"O";
                        }
                    }
                    cout<<endl;
                }
                for (k=0;k<indentacion;k++){
                    cout << " ";
                }
                cout<<"trn "<<actual->turno<<" g/p "<<actual->g_p<<" uct "<<actual->uct<<" vsts "<<actual->n<<" wns "<<actual->wins<<"\n"<<endl;
            }
        }

        //liberacion de la memoria
        void free_tree(node* actual){
            if(actual!=NULL){
                int i;
                for(i=0;i<actual->hijos.size();i++){
                    free_tree(actual->hijos[i]);
                }
                delete(actual);
            }
        }

        void calcular_uct(node *nodo, double constante=2){
            if(nodo!=NULL){
                //nodo->uct=(nodo->pad!=NULL)?((nodo->n==0)?(0):((nodo->wins/nodo->n)+sqrt((constante*log(nodo->pad->n))/(nodo->n)))):((nodo->n==0)?(0):(nodo->wins/nodo->n));
                if(nodo->pad!=NULL){
                    if(nodo->n!=0){
                        nodo->uct=((nodo->wins*1.0)/(nodo->n*1.0))+sqrt((constante*log(nodo->pad->n))/(nodo->n*1.0));
                    }
                    else{
                        nodo->uct=6.0;
                    }
                }
                else{
                    if(nodo->n!=0){
                        nodo->uct=((nodo->wins*1.0)/(nodo->n*1.0));
                    }
                    else{
                        nodo->uct=6.0;
                    }
                }
            }
            /*la lógica de este paso es:
            si el nodo no es nulo:

                si el padre del nodo no es nulo:
                    si visitas distinto de cero:
                        nodo->uct=formula completa
                    demás:
                        nodo->uct=0
                demás: 
                    si visitas distinto de cero:
                        nodo->uct=promedio ponderado (solamente wins entre visitas)
                    demás:
                        nodo->uct=0
            */
        }

        tuple<vector<vector<int> >, int> montecarlo_tree_search_random_search(vector<vector<int> > tablero, int turno, int tamano, double exploracion=2, bool verbose=false){
            /*
            if(raiz!=NULL){
                free_tree(raiz);
            }*/
            vector<vector<int> > tablero_copia=tablero;
            int turno_copia=turno;
            if(verbose){
                cout<<"TURNO COPIA "<<turno_copia<<endl;
                printf("____________________________0\n");
            }
            if(!hexa.condiciones_de_terminado(tablero)){
                raiz=new node(tablero, turno, 0);
                node *nodo_desechable=NULL;
                node *nodo_backprop=NULL;
                int nodos_cantidad=0, i, j;
                tuple<vector<vector<int> >, int> tupla;
                vector<vector<int> >tiro;
                int turno_siguiente;
                int resultado;
                
                

                
                tupla=hexa.tiro_random(tablero, turno);
                tiro=get<0>(tupla);
                turno_siguiente=get<1>(tupla);
                nodo_desechable=insertar(raiz, tiro, turno_siguiente, (hexa.condiciones_de_terminado(tiro))?((turno_siguiente==1)?(2):(1)):(0));
                if(verbose){
                    printf("____________________________1\n");
                }
                if(turno_copia==hexa.terminar_juego(tiro, turno_siguiente)){
                    nodo_desechable->wins++;
                    nodo_desechable->n++;
                    raiz->wins++;
                    raiz->n++;
                }
                else{
                    nodo_desechable->n++;
                    raiz->n++;
                }
                if(verbose){
                    printf("____________________________2\n");
                }
                calcular_uct(nodo_desechable, exploracion);
                calcular_uct(raiz, exploracion);
                nodo_desechable=NULL;
                if(verbose){
                    printf("____________________________4\n");
                }
                while(nodos_cantidad<tamano){
                    nodo_desechable=raiz;
                    tablero=nodo_desechable->tablero;
                    turno=nodo_desechable->turno;
                    while(!nodo_desechable->hijos.empty()){
                        tupla=hexa.tiro_random(tablero, turno);
                        tiro=get<0>(tupla);
                        turno_siguiente=get<1>(tupla);
                        for(i=0;i<nodo_desechable->hijos.size();i++){
                            if(nodo_desechable->hijos[i]->tablero==tiro){
                                nodo_desechable=nodo_desechable->hijos[i];
                                break;
                            }
                            if(i==nodo_desechable->hijos.size()-1){
                                i=-1;
                                break;
                            }
                        }
                        //SELECCION
                        if(i==-1){
                            if(turno_siguiente!=0){
                                //EXPANSION
                                nodo_desechable=insertar(nodo_desechable, tiro, turno_siguiente, (hexa.condiciones_de_terminado(tiro))?((turno_siguiente==1)?(2):(1)):(0));
                                nodo_backprop=nodo_desechable;
                                //resultado=(((turno_copia==1)?(2):(1))==hexa.terminar_juego(tiro, turno_siguiente))?(1):(0);
                                if(nodo_backprop->g_p==turno_copia){
                                    resultado=1;
                                }
                                else if(nodo_backprop->g_p!=0){
                                    resultado=0;
                                }
                                else{
                                    resultado=(turno_copia==hexa.terminar_juego(tiro, turno_siguiente))?(1):(0);
                                }
                                nodo_backprop->n++;
                                if(resultado==1){
                                    nodo_backprop->wins++;
                                }
                                while(nodo_backprop->pad!=NULL){
                                    nodo_backprop->pad->wins+=resultado;
                                    nodo_backprop->pad->n++;
                                    calcular_uct(nodo_backprop, exploracion);
                                    nodo_backprop=nodo_backprop->pad;
                                }
                                calcular_uct(nodo_backprop, exploracion);
                            }
                            else{
                                break;
                            }
                        }
                        else{
                            //if no tiene hijos, darle uno, y regresar a la raiz
                            if(nodo_desechable->g_p!=0){
                                break;
                            }
                            else if(nodo_desechable->hijos.empty()){
                                tupla=hexa.tiro_random(tiro, turno_siguiente);
                                tiro=get<0>(tupla);
                                turno_siguiente=get<1>(tupla);
                                //EXPANSION
                                nodo_backprop=insertar(nodo_desechable, tiro, turno_siguiente, (hexa.condiciones_de_terminado(tiro))?((turno_siguiente==1)?(2):(1)):(0));
                                //resultado=(((turno_copia==1)?(2):(1))==hexa.terminar_juego(tiro, turno_siguiente))?(1):(0);
                                if(nodo_backprop->g_p==turno_copia){
                                    resultado=1;
                                }
                                else if(nodo_backprop->g_p!=0){
                                    resultado=0;
                                }
                                else{
                                    resultado=(turno_copia==hexa.terminar_juego(tiro, turno_siguiente))?(1):(0);
                                }
                                nodo_backprop->n++;
                                nodo_backprop->wins+=resultado;
                                while(nodo_backprop->pad!=NULL){
                                    nodo_backprop->pad->wins+=resultado;
                                    nodo_backprop->pad->n++;
                                    calcular_uct(nodo_backprop, exploracion);
                                    nodo_backprop=nodo_backprop->pad;
                                }
                                calcular_uct(nodo_backprop, exploracion);
                                nodo_desechable=raiz;
                                tablero=tablero_copia;
                                turno=turno_copia;
                                break;
                            }
                            else{
                                tablero=nodo_desechable->tablero;
                                turno=hexa.siguiente_turno(turno);
                            }
                        }
                    }
                    nodos_cantidad++;
                }
                if(verbose){
                    fancy_print(raiz);
                }
                double max=0;
                if(raiz->hijos.size()==1){
                    tablero=raiz->hijos[0]->tablero;
                }
                else{
                    for(i=0;i<raiz->hijos.size();i++){
                        if(raiz->hijos[i]->uct>max){
                            max=raiz->hijos[i]->uct;
                            tablero=raiz->hijos[i]->tablero;
                        }
                    }
                }
                free_tree(raiz);
                return make_tuple(tablero,(turno_copia==1)?(2):(1));
            }
            else{
                if(verbose){
                    cout<<"NO_POSSIBLE_SHOT"<<endl;
                }
                return make_tuple(tablero,turno);
            }
        }

        tuple<vector<vector<int> >, int> montecarlo_tree_search_probabilistic_search(vector<vector<int> > tablero, int turno, int tamano, double exploracion=2, bool verbose=false){
            /*
            if(raiz!=NULL){
                free_tree(raiz);
            }*/
            vector<vector<int> > tablero_copia=tablero;
            int turno_copia=turno;
            if(verbose){
                cout<<"TURNO COPIA "<<turno_copia<<endl;
                printf("____________________________0\n");
            }
            if(!hexa.condiciones_de_terminado(tablero)){
                raiz=new node(tablero, turno, 0);
                node *nodo_desechable=NULL;
                node *nodo_backprop=NULL;
                int nodos_cantidad=0, i, j;
                double max=0;
                tuple<vector<vector<int> >, int> tupla;
                vector<vector<int> >tiro;
                int turno_siguiente;
                int resultado;
                if(verbose){
                    printf("____________________________1\n");
                }
                if(verbose){
                    printf("____________________________2\n");
                }
                if(verbose){
                    printf("____________________________4\n");
                }
                //a la raiz, darle todos los hijos que pueda tener, y darles uct;
                tuple< vector<vector<vector<int> > >,int> todos=hexa.todos_los_tiros(tablero,turno);
                vector<vector<vector<int> > > tableros=get<0>(todos);
                int turnos=get<1>(todos);
                for(i=0;i<tableros.size();i++){
                    nodo_backprop=insertar(raiz, tableros[i] ,turnos, (hexa.condiciones_de_terminado(tableros[i]))?((turnos==1)?(2):(1)):(0));
                    calcular_uct(nodo_backprop, exploracion);
                }
                calcular_uct(raiz, exploracion);
                nodo_desechable=NULL;
                while(nodos_cantidad<tamano){
                    nodo_desechable=raiz;
                    //SELECCION
                    while(!nodo_desechable->hijos.empty()){
                        for(i=0;i<nodo_desechable->hijos.size();i++){
                            calcular_uct(nodo_desechable->hijos[i], exploracion);
                        }
                        max=0.0;
                        for(i=0;i<nodo_desechable->hijos.size();i++){
                            if(nodo_desechable->hijos[i]->uct>max){
                                max=nodo_desechable->hijos[i]->uct;
                                j=i;
                            }
                        }
                        nodo_desechable=nodo_desechable->hijos[j];
                    }
                    if(nodo_desechable!=NULL){
                        if(!hexa.condiciones_de_terminado(nodo_desechable->tablero)){
                            //EXPANSION
                            todos=hexa.todos_los_tiros(nodo_desechable->tablero,(nodo_desechable->turno==1)?(2):(1));
                            tableros=get<0>(todos);
                            turnos=get<1>(todos);
                            if(!tableros.empty()){
                                for(i=0;i<tableros.size();i++){
                                    insertar(nodo_desechable, tableros[i], turnos, (hexa.condiciones_de_terminado(tableros[i]))?((turnos==1)?(1):(2)):(0));
                                }
                                nodo_backprop=nodo_desechable->hijos[nodo_desechable->hijos.size()-1];
                                //SIMULACION
                                if(nodo_backprop->g_p==turno_copia){
                                    resultado=1;
                                }
                                else if(nodo_backprop->g_p!=0){
                                    resultado=0;
                                }
                                else{
                                    resultado=(turno_copia==hexa.terminar_juego(nodo_backprop->tablero, turnos))?(1):(0);
                                }
                                //BACKPROP
                                while(nodo_backprop->pad!=NULL){
                                    nodo_backprop->pad->wins+=resultado;
                                    nodo_backprop->pad->n++;
                                    nodo_backprop=nodo_backprop->pad;
                                }
                            }
                        }
                        else{
                            if(nodo_desechable->g_p==turno_copia){
                                resultado=1;
                            }
                            else{
                                resultado=0;
                            }
                            //BACKPROP
                            nodo_backprop=nodo_desechable;
                            //nodo_backprop->n++;
                            //nodo_backprop->wins+=resultado;
                            while(nodo_backprop->pad!=NULL){
                                nodo_backprop->pad->wins+=resultado;
                                nodo_backprop->pad->n++;
                                nodo_backprop=nodo_backprop->pad;
                            }
                        }
                    }
                    nodos_cantidad++;
                }
                if(verbose){
                    fancy_print(raiz);
                }
                max=0;
                if(raiz->hijos.size()==1){
                    tablero=raiz->hijos[0]->tablero;
                }
                else{
                    for(i=0;i<raiz->hijos.size();i++){
                        if(raiz->hijos[i]->uct>max){
                            max=raiz->hijos[i]->uct;
                            tablero=raiz->hijos[i]->tablero;
                        }
                    }
                }
                free_tree(raiz);
                return make_tuple(tablero,(turno_copia==1)?(2):(1));
            }
            else{
                if(verbose){
                    cout<<"NO_POSSIBLE_SHOT"<<endl;
                }
                return make_tuple(tablero,turno);
            }
        }

        int leer_entero() {
            int x;
            while (1) {
                if (cin >> x) {
                    if(x==123456789){
                        exit(2);
                    }
                    return x;
                } 
                else{
                    cin.clear(); // limpia error
                    cin.ignore(10000, '\n'); // descarta entrada hasta el enter
                    cout << "Entrada inválida. Intenta de nuevo: ";
                }
            }
        }

        double leer_double() {
            double x;
            while (1) {
                if (cin >> x) {
                    if(x==123456789){
                        exit(1);
                    }
                    return x;
                } 
                else{
                    cin.clear(); // limpia error
                    cin.ignore(10000, '\n'); // descarta entrada hasta el enter
                    cout << "Entrada inválida. Intenta de nuevo: ";
                }
            }
        }

        void _juego_contra_humano_inteligente(int tamano, int difaul=0, bool primero=0, bool verbose=0, double dificultad=-1, bool tipo_de_busqueda=true){
            cout<<"JUEGO CONTRA HUMANO:"<<endl;
            tuple<vector<vector<int> >, int> inicio=hexa.poner_el_juego(tamano, difaul);
            vector<vector<int> > tablero=get<0>(inicio);
            int turno=get<1>(inicio);
            int seguir=1, f_o, c_o, f_t, c_t, victoria_humana=0, victoria_maquina=0;
            hexa.ver_tablero(tablero,turno);
            while(seguir){
                if(primero){
                    cout<<"\ntiro de maquina:\n";
                    turno=hexa.siguiente_turno(turno);
                    if(tipo_de_busqueda){
                        inicio=montecarlo_tree_search_probabilistic_search(tablero,turno, (dificultad==-1)?(50000):((int)(dificultad*(0.02*pow(93.57,tablero.size())))), hexa.siguiente_turno(turno), verbose);
                    }
                    else{
                        inicio=montecarlo_tree_search_random_search(tablero,turno, (dificultad==-1)?(50000):((int)(dificultad*(0.02*pow(93.57,tablero.size())))), hexa.siguiente_turno(turno), verbose);
                    }
                    tablero=get<0>(inicio);
                    turno=get<1>(inicio);
                    hexa.ver_tablero(tablero,turno);
                }
                while(seguir){
                    while(1){
                        cout<<"\n(codigo de salida: 123456789)\n\nFilas(origen):";
                        f_o=leer_entero();
                        cout<<"\nColumnas(origen):";
                        c_o=leer_entero();
                        cout<<"\nFilas(terminal):";
                        f_t=leer_entero();
                        cout<<"\nColumnas(terminal):";
                        c_t=leer_entero();
                        if(hexa.tiro_legal(f_o,c_o,f_t,c_t,tablero)&&(tablero[f_o][c_o]==(difaul==0)?(1):(difaul))){
                            tablero[f_o][c_o]=0;
                            tablero[f_t][c_t]=(difaul==0)?(1):(difaul);
                            break;
                        }
                        else{
                            cout<<"\nTiro no permitido, intenta de nuevo.";
                        }
                    }
                    turno=hexa.siguiente_turno(turno);
                    hexa.ver_tablero(tablero,turno);
                    if(hexa.condiciones_de_terminado(tablero)){
                        cout<<"\nVICTORIA DE:";
                        if(turno==1){
                            (difaul==0)?(victoria_maquina++):((difaul==1)?(victoria_maquina++):(victoria_humana++));
                            cout<<"O\n\n-------------------------------MARCADOR------------------------------\n";
                            cout<<"╔═══════════════════════════════════════════════════════════════════╗\n";
                            cout<<"║ Humano:      "<<setw(3)<< left << victoria_humana<<"                                                  ║\n";
                            if(victoria_humana>victoria_maquina){
                                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: HUMANO ║\n";
                            }
                            else if(victoria_humana<victoria_maquina){
                                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: MAQUINA║\n";
                            }
                            else{
                                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: TABLAS ║\n";
                            }
                            cout<<"║ Computadora: "<<setw(3)<< left << victoria_maquina<<"                                                  ║\n";
                            cout<<"╚═══════════════════════════════════════════════════════════════════╝\n";
                            cout<<"\n\n\nQuieres seguir?:(1=si, 0=no)";
                        }
                        else{
                            (difaul==0)?(victoria_humana++):((difaul==1)?(victoria_humana++):(victoria_maquina++));
                            cout<<"X\n\n-------------------------------MARCADOR------------------------------\n";
                            cout<<"╔═══════════════════════════════════════════════════════════════════╗\n";
                            cout<<"║ Humano:      "<<setw(3)<< left << victoria_humana<<"                                                  ║\n";
                            if(victoria_humana>victoria_maquina){
                                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: HUMANO ║\n";
                            }
                            else if(victoria_humana<victoria_maquina){
                                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: MAQUINA║\n";
                            }
                            else{
                                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: TABLAS ║\n";
                            }
                            cout<<"║ Computadora: "<<setw(3)<< left << victoria_maquina<<"                                                  ║\n";
                            cout<<"╚═══════════════════════════════════════════════════════════════════╝\n";
                            cout<<"\n\n\nQuieres seguir?:(1=si, 0=no)";
                        }
                        cin>>seguir;
                        if(seguir){
                            inicio=hexa.poner_el_juego(tamano);
                            tablero=get<0>(inicio);
                            turno=get<1>(inicio);
                            hexa.ver_tablero(tablero,turno);
                            break;
                        }
                        else{
                            if(turno==1){
                                cout<<"\nGracias por jugar, suerte para la próxima! :)";
                            }
                            else{
                                cout<<"\nGracias por jugar, mejoraré para ganarte la próxima! >:)";
                            }
                        }
                    }
                    else{
                        cout<<"\ntiro de maquina:\n";
                        if(dificultad!=-1){
                            cout<<"nodos a calcular: "<<(int)(dificultad*(0.02*pow(93.57,tablero.size())))<<endl;
                        }
                        else{
                            cout<<"nodos a calcular: 50000"<<endl;
                        }
                        if(tipo_de_busqueda){
                            inicio=montecarlo_tree_search_probabilistic_search(tablero,turno, (dificultad==-1)?(50000):((int)(dificultad*(0.02*pow(93.57,tablero.size())))), hexa.siguiente_turno(turno), verbose);
                        }
                        else{
                            inicio=montecarlo_tree_search_random_search(tablero,turno, (dificultad==-1)?(50000):((int)(dificultad*(0.02*pow(93.57,tablero.size())))), hexa.siguiente_turno(turno), verbose);
                        }
                        tablero=get<0>(inicio);
                        turno=get<1>(inicio);
                        hexa.ver_tablero(tablero,turno);
                        if(hexa.condiciones_de_terminado(tablero)){
                            cout<<"\nVICTORIA DE:";
                            if(turno==1){
                                (difaul==0)?(victoria_maquina++):((difaul==1)?(victoria_maquina++):(victoria_humana++));
                                cout<<"O\n\n-------------------------------MARCADOR------------------------------\n";
                                cout<<"╔═══════════════════════════════════════════════════════════════════╗\n";
                                cout<<"║ Humano:      "<<setw(3)<< left << victoria_humana<<"                                                  ║\n";
                                if(victoria_humana>victoria_maquina){
                                    cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: HUMANO ║\n";
                                }
                                else if(victoria_humana<victoria_maquina){
                                    cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: MAQUINA║\n";
                                }
                                else{
                                    cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: TABLAS ║\n";
                                }
                                cout<<"║ Computadora: "<<setw(3)<< left << victoria_maquina<<"                                                  ║\n";
                                cout<<"╚═══════════════════════════════════════════════════════════════════╝\n";
                                cout<<"\n\n\nQuieres seguir?:(1=si, 0=no)";
                                
                            }
                            else{
                                (difaul==0)?(victoria_humana++):((difaul==1)?(victoria_humana++):(victoria_maquina++));
                                cout<<"X\n\n-------------------------------MARCADOR------------------------------\n";
                                cout<<"╔═══════════════════════════════════════════════════════════════════╗\n";
                                cout<<"║ Humano:      "<<setw(3)<< left << victoria_humana<<"                                                  ║\n";
                                if(victoria_humana>victoria_maquina){
                                    cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: HUMANO ║\n";
                                }
                                else if(victoria_humana<victoria_maquina){
                                    cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: MAQUINA║\n";
                                }
                                else{
                                    cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: TABLAS ║\n";
                                }
                                cout<<"║ Computadora: "<<setw(3)<< left << victoria_maquina<<"                                                  ║\n";
                                cout<<"╚═══════════════════════════════════════════════════════════════════╝\n";
                                cout<<"\n\n\nQuieres seguir?:(1=si, 0=no)";
                            }
                            cin>>seguir;
                            if(seguir){
                                inicio=hexa.poner_el_juego(tamano);
                                tablero=get<0>(inicio);
                                turno=get<1>(inicio);
                                hexa.ver_tablero(tablero,turno);
                                break;
                            }
                            else{
                                if(turno==1){
                                    cout<<"\nGracias por jugar, suerte para la próxima! :)\n";
                                }
                                else{
                                    cout<<"\nGracias por jugar, mejoraré para ganarte la próxima! >:)\n";
                                }
                            }
                        }
                    }
                }
            }
            //cout<<"\n\nMARCADOR FINAL:\n-------------------------------------------\n-Victorias humanas        :"<<victoria_humana<<"\n-Victorias de computadora :"<<victoria_maquina<<"\n-------------------------------------------\n";
            cout<<"----------------------------MARCADOR FINAL---------------------------\n";
            cout<<"╔═══════════════════════════════════════════════════════════════════╗\n";
            cout<<"║ Humano:      "<<setw(3)<< left << victoria_humana<<"                                                  ║\n";
            if(victoria_humana>victoria_maquina){
                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: HUMANO ║\n";
            }
            else if(victoria_humana<victoria_maquina){
                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: MAQUINA║\n";
            }
            else{
                cout<<"║                             Razón:"<< fixed << setprecision(2)<<(victoria_humana*1.0)/(victoria_maquina*1.0)<<"            Ventaja: TABLAS ║\n";
            }
            cout<<"║ Computadora: "<<setw(3)<< left << victoria_maquina<<"                                                  ║\n";
            cout<<"╚═══════════════════════════════════════════════════════════════════╝\n";
            if(victoria_humana==victoria_maquina){
                cout<<"-EMPATE\n\n";
            }
            else if(victoria_humana>victoria_maquina){
                cout<<"-GANASTE\n\n";
            }
            else{
                cout<<"-PERDISTE\n\n";
            }
        }

        void juego_contra_humano_inteligente(){
            cout<<"\n\nQuieres el juego default? (1=si, 0=entrar a configuracion)";
            int setinds=leer_entero();
            if(setinds==0){
                cout<<"\n\nQué piezas quieres tener? (X=1, O=2)  ";
                int piezas=leer_entero();
                cout<<"\n\nQuién quieres que inicie primero? (tu=0, computadora=1) ";
                bool primero=(leer_entero()!=0)?(true):(false);
                cout<<"\n\nQué tamaño quieres que tenga el tablero?";
                int tamanio=leer_entero();
                tamanio=(tamanio<3)?(3):(tamanio);
                cout<<"\n\nQuieres ver el arbol de probabilidad? (1=si, 0=no) (RECOMENDABLE NO VERLO, PUEDEN SER ARBOLES MUY GRANDES)";
                int ver_=(leer_entero()!=0)?(true):(false);
                cout<<"\n\nQué dificultad quieres para el juego? (valor entre 0 y 1, 0 es muy facil, 1 es muy dificil)";
                double dificultad=leer_double();
                cout<<"\n\nQué tipo de búsqueda quieres que se haga? (random=0, probabilistica=1) ";
                bool busq=(leer_entero()!=0)?(true):(false);
                _juego_contra_humano_inteligente(tamanio, piezas, primero, ver_, dificultad, busq);
            }
            else{
                _juego_contra_humano_inteligente(3, 1, false, false, -1, true);
            }
        }
};

int main(){
    tree clasi(NULL);
    clasi.juego_contra_humano_inteligente();
    return 0;
}