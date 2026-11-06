import SwiftUI

/// 3D Parallax Tilt Effect with Dynamic Specular Lighting (Apple TV / tvOS style).
struct ParallaxTiltModifier: ViewModifier {
    var maxTilt: Double = 14.0
    var enableGlare: Bool = true

    @State private var tiltX: Double = 0.0
    @State private var tiltY: Double = 0.0
    @State private var isHovered: Bool = false

    func body(content: Content) -> some View {
        GeometryReader { geo in
            let w = geo.size.width
            let h = geo.size.height

            ZStack {
                content
                    .rotation3DEffect(
                        .degrees(-tiltY * maxTilt),
                        axis: (x: 1, y: 0, z: 0),
                        perspective: 0.6
                    )
                    .rotation3DEffect(
                        .degrees(tiltX * maxTilt),
                        axis: (x: 0, y: 1, z: 0),
                        perspective: 0.6
                    )

                if enableGlare && isHovered {
                    // Dynamic specular glare that shifts opposite to the tilt
                    RadialGradient(
                        colors: [
                            Color.white.opacity(0.35),
                            Color.white.opacity(0.08),
                            Color.clear
                        ],
                        center: UnitPoint(
                            x: 0.5 - (tiltX * 0.4),
                            y: 0.5 - (tiltY * 0.4)
                        ),
                        startRadius: 10,
                        endRadius: max(w, h) * 0.65
                    )
                    .blendMode(.overlay)
                    .allowsHitTesting(false)
                }
            }
            .scaleEffect(isHovered ? 1.03 : 1.0)
            .animation(.interactiveSpring(response: 0.35, dampingFraction: 0.72), value: isHovered)
            .animation(.interactiveSpring(response: 0.25, dampingFraction: 0.85), value: tiltX)
            .animation(.interactiveSpring(response: 0.25, dampingFraction: 0.85), value: tiltY)
            .onContinuousHover { phase in
                switch phase {
                case .active(let location):
                    isHovered = true
                    let normX = (location.x / w) * 2.0 - 1.0
                    let normY = (location.y / h) * 2.0 - 1.0
                    tiltX = min(max(normX, -1.0), 1.0)
                    tiltY = min(max(normY, -1.0), 1.0)
                case .ended:
                    isHovered = false
                    tiltX = 0.0
                    tiltY = 0.0
                }
            }
        }
    }
}

extension View {
    func parallaxTilt(maxTilt: Double = 14.0, enableGlare: Bool = true) -> some View {
        self.modifier(ParallaxTiltModifier(maxTilt: maxTilt, enableGlare: enableGlare))
    }
}
