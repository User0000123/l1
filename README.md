inline void vector_cross_product3(gsl_vector *v1, gsl_vector *v2, gsl_vector *result) /* Вычисление векторного произведения */
inline int isInside(gsl_vector *pPoint) /* Проверка на отсечение точки в CLIPSPACE */
void ApplyTransformations() /* Создание матрицы преобразования в видовые координаты. В цикле получение координат для CLIPSPACE и проверка на отсечение(если отсекаем, то дальше не перемножаем). Хотя нужно будет убрать, если переделывать рисование треугольников. */
void InitializeResources() /* Просто начальная инициализация почти всех векторов, массивов и т.д. (часть будет realloc в WM_SIZE). Тут же инициализируется пул стандартный пул потоков винды. */
void FreeAllResources() /* Ну это понятно. Можно вообще этого не делать... */
inline void DrawLine(HDC dc, int x0, int y0, int x1, int y1, byte r, byte g, byte b) /* Отрисовка линии по Брезензему */
inline void DrawTriangle(PARAMS *pThreadParams, HDC dc, gsl_vector_int *pTriangleVertices, byte r, byte g, byte b) /* Отрисовка треугольника */
void SetFullScreen(HWND hwnd) /* Включение, отключение полного экрана на Alt+F12 */
void flatShading(int index, PARAMS *param, double *intens) /* Плоское затенение */
void CALLBACK task(PTP_CALLBACK_INSTANCE Instance, PVOID params, PTP_WORK Work) /* Таска для отрисовки треугольников в пуле потоков */
void DrawProc(HDC hdc) /* Процедура отрисовки всей картинки на заданном HDC */
void MoveProc() /* Перемещение камеры на кнопки WASD */