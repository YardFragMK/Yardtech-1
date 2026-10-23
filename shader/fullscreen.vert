#version 120

// Tam ekran son-islem gecisleri icin ortak vertex shader: ortografik bir
// dortgen cizilir, doku koordinati 0..1 ekran uzayidir.
void main() {
    gl_Position = ftransform();
    gl_TexCoord[0] = gl_MultiTexCoord0;
}