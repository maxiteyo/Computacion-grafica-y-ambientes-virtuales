// ============================================================================
// CameraCommand.h — Comandos del usuario, ya interpretados, en esféricas.
//
// El usuario mueve el mouse con coordenadas de PANTALLA (2D); quien las lee es
// InputHandler y las convierte a CAMBIOS de estado de la órbita. CameraSystem
// solo recibe estos deltas: no sabe nada del hardware de entrada.
//   - yaw_delta   : [rad] cambio de longitud (acimut) acumulado en el cuadro.
//   - pitch_delta : [rad] cambio de latitud (elevación).
//   - dist_delta  : [unidades de escena] (+ se aleja, - se acerca al objetivo).
// ============================================================================
#pragma once

struct CameraCommand {
    float yaw_delta   = 0.0f;
    float pitch_delta = 0.0f;
    float dist_delta  = 0.0f;
};