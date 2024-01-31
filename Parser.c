#include <gsl/gsl_matrix.h>
#include <stdio.h>

int main()
{
    gsl_vector *vector = gsl_vector_alloc(3);
    FILE *file;
    
    file = fopen("C:\\Users\\Aleksej\\Desktop\\labs\\ACG\\l1\\t.txt", "r");

    // double v[] = {1, 1, 1};
    // vector = gsl_vector_view_array(v, 3).vector;
    // vector.size = 3;
    printf("%d", gsl_vector_fscanf(file, vector));
    gsl_vector_fprintf(stdout, vector, "%f");
    
    fclose(file);
}

void *parceVerteces(FILE *stream)
{
    
}

void *parcePolygons(FILE *stream)
{

}