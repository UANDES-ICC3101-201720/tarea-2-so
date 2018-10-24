# Tarea 2 - Informe
> Pedro Ricci

Detalles de como funciona cada método en comentarios en el [main.c](main.c)
## Hipótesis
El algoritmo custom es una versión mejorada de FIFO.
Se espera que performe de igual o mejor manera. 
El algoritmo random debe ser mejor que FIFO, ya que FIFO reemplaza muchas veces la página que fue usada últimamente, lo que no es buena estrategia.
## Gráficos
Colores: 
### Sort
#### Faltas de página
![](graphs/sort-pf.png)
#### Lecturas de disco
![](graphs/sort_dr.png)
#### Escrituras en disco
![](graphs/sort_dw.png)


### Scan
#### Faltas de página
![](graphs/scan_pf.png)
#### Lecturas de disco
![](graphs/scan_dr.png)
#### Escrituras en disco
![](graphs/scan_dw.png)

### Focus
#### Faltas de página
![](graphs/focus_pf.png)
#### Lecturas de disco
![](graphs/focus_dr.png)
#### Escrituras en disco
![](graphs/focus_dw.png)
