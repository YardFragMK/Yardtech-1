#version 120

// Dunya yuzeyleri icin dinamik isik shader'i. Konumlar goz uzayinda hesaplanir; boylece
// isik konumlari CPU'da ayni uzaya cevrilerek verilir ve ters matris gerekmez.
varying vec3 vEyePos;
varying vec3 vNormal;

void main() {
    vEyePos = (gl_ModelViewMatrix * gl_Vertex).xyz;
    vNormal = gl_NormalMatrix * gl_Normal;

    // ftransform: sabit fonksiyonlu cizimle birebir ayni konum (komsu yuzeyler farkli
    // yoldan cizildiginde derinlik cakismasi/catlak olmasin).
    gl_Position = ftransform();

    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_TexCoord[1] = gl_MultiTexCoord1;
    gl_FrontColor = gl_Color;
}