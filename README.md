<br />
<p align="center">
  <h3 align="center">Sistemas Operativos - TP 2C 2019</h3>
  <h3 align="center">Los Borbotones</h3>   
</p>
<br />

### Instalación

1. Desde la terminal ir al workspace
```sh
cd workspace
```
2. Clonar el repo
```sh
git clone https://github.com/sisoputnfrba/tp-2019-2c-Los-Borbotones
```
3. Ejecutar el deployer ubicado en el directorio principal del repo
```sh
sudo ./deploy
```
_El deployer instala las commons y las librerias compartidas._

4. Compilar y ejecutar el proceso deseado
```sh
cd <modulo>/<proceso>
make all
./<nombre_del_proceso> <comando_de_ejecucion>
```
_El comando de ejecucion es opcional_
