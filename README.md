**Sistem Solar Interactiv 3D**

Acest proiect reprezinta o simulare tridimensionala a Sistemului Solar, dezvoltata folosind limbajul C++ si OpenGL. Obiectivul principal al aplicatiei este de a oferi o reprezentare vizuala si interactiva a corpurilor ceresti, permitand utilizatorului sa exploreze orbitele, sa observe miscarea planetelor si sa obtina informatii detaliate despre fiecare corp ceresc printr-o interfata intuitiva.

**Elemente Incluse:**

Aplicatia integreaza urmatoarele elemente fundamentale si functionale:

- **Corpurile Ceresti:** Soarele si cele 8 planete majore, fiecare avand proprietati unice (raza, distanta, viteza de rotatie, culoare).
- **Geometrie Avansata:** Planetele sunt randate ca sfere de inalta rezolutie , iar Saturn si Uranus dispun de inele generate prin geometrie de tip disc.
- **Centura de Asteroizi:** Un sistem de particule care simuleaza centura dintre Marte si Jupiter, rotindu-se independent in jurul Soarelui.
- **Sistem de Iluminare:** O sursa de lumina situata in centrul sistemului (Soarele) care ilumineaza corect fetele planetelor.

**Interactivitate:**

- - Navigare libera cu mouse-ul (rotire si zoom).
    - Sistem de selectie pentru a da click pe planete.
    - Interfata grafica care afiseaza numele, temperatura si masa planetei selectate.
    - Controlul timpului (pauza, accelerare, decelerare).

**Originalitate:**

- **Texturare Procedurala:** Planetele nu folosesc imagini (.jpg) lipite pe ele. Aspectul lor (de exemplu, dungile de pe Jupiter sau continentele simulate de pe Pamant) este generat in timp real in Fragment Shader folosind functii matematice (Sinus, Cosinus, Mix).
- **Matematica Proprie:** Intreaga biblioteca de matematica necesara pentru grafica 3D (matrici de transformare 4x4, proiectii, camera) a fost implementata manual in cod.
- **Randare Hibrida:** Combinarea randarii 3D moderne cu o interfata 2D suprapusa corect, gestionand manual Depth Buffer-ul pentru a evita erorile vizuale.

**1\. Niculae Cosmin**

- **Arhitectura Aplicatiei:** Initializarea contextului OpenGL si configurarea ferestrei folosind GLUT.
- **Generare Geometrie:** Implementarea algoritmilor pentru generarea formelor 3D (sfere pentru planete, discuri pentru inele, puncte pentru stele).
- **Control Camera:** Implementarea sistemului de navigare in spatiu 3D (rotire, zoom) si gestionarea input-ului de la tastatura si mouse.

**2\. Branzea Malina**

- **Programare Shadere:** Scrierea si integrarea shaderelor de Vertex si Fragment.
- **Texturare Procedurala:** Implementarea algoritmilor matematici in shader pentru a genera suprafata planetelor (benzile lui Jupiter, continentele Pamantului) fara a folosi imagini.
- **Iluminare:** Implementarea modelului de iluminare Phong si a efectului de atmosfera (Rim Lighting).

**3\. Calin Bantas**

- **Biblioteca Matematica:** Implementarea manuala a structurii de matrici 4x4 si a functiilor de transformare (translatie, rotatie, scalare, perspectiva), esentiale pentru Modern OpenGL.
- **Logica Sistemului Solar:** Configurarea datelor astronomice si a logicii pentru centura de asteroizi.
- **Interactivitate si UI:** Implementarea algoritmului de "Ray Casting" pentru detectia click-ului pe planete si afisarea interfetei grafice (HUD) cu informatii.

Cod: <https://github.com/Cosmin3l28/SolarSystem>
