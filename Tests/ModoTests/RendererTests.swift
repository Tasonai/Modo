import AppKit
import ModoKit
import Testing
@testable import Modo

/// Solid red full-bleed square, like an icon with no transparent margin.
private func redSquare(_ size: Int = 256) -> NSImage {
    let image = NSImage(size: NSSize(width: size, height: size))
    image.lockFocus()
    NSColor(srgbRed: 0.9, green: 0.1, blue: 0.1, alpha: 1).setFill()
    NSRect(x: 0, y: 0, width: size, height: size).fill()
    image.unlockFocus()
    return image
}

private func render(_ style: RoundIconStyle, size: Int = 256) -> NSBitmapImageRep {
    let image = IconRenderer.renderRoundIcon(sourceImage: redSquare(), style: style, fallbackHex: 0xE53935, canvasSize: size)
    let cg = image.cgImage(forProposedRect: nil, context: nil, hints: nil)!
    return NSBitmapImageRep(cgImage: cg)
}

private func pixel(_ rep: NSBitmapImageRep, _ fx: Double, _ fy: Double) -> NSColor {
    let x = min(rep.pixelsWide - 1, Int(fx * Double(rep.pixelsWide)))
    let y = min(rep.pixelsHigh - 1, Int(fy * Double(rep.pixelsHigh)))
    return rep.colorAt(x: x, y: y)!.usingColorSpace(.sRGB)!
}

@Test("Canvas corners stay transparent and the center is opaque for every shape", arguments: IconShape.allCases)
func shapeAlpha(_ shape: IconShape) {
    let rep = render(RoundIconStyle(shape: shape, shadowIntensity: 0))
    #expect(pixel(rep, 0.02, 0.02).alphaComponent < 0.01)
    #expect(pixel(rep, 0.98, 0.98).alphaComponent < 0.01)
    #expect(pixel(rep, 0.5, 0.5).alphaComponent > 0.99)
}

@Test("A hexagon leaves the circle's left edge empty, a circle does not")
func hexagonVersusCircle() {
    let probe = (x: 0.10, y: 0.5)
    let circle = render(RoundIconStyle(shape: .circle, shadowIntensity: 0))
    let hexagon = render(RoundIconStyle(shape: .hexagon, shadowIntensity: 0))
    #expect(pixel(circle, probe.x, probe.y).alphaComponent > 0.9)
    #expect(pixel(hexagon, probe.x, probe.y).alphaComponent < 0.05)
}

@Test("Drop shadow adds soft pixels outside the shape")
func shadowOutsideShape() {
    let flat = render(RoundIconStyle(shape: .circle, shadowIntensity: 0))
    let shadowed = render(RoundIconStyle(shape: .circle, shadowIntensity: 1))
    // Just below the circle's bottom edge (margin is 7.5% of the canvas).
    #expect(pixel(flat, 0.5, 0.06).alphaComponent < 0.01)
    #expect(pixel(shadowed, 0.5, 0.06).alphaComponent > 0.01)
}

@Test("Monochrome mode leaves no colour cast on a red icon")
func monochromeIsGray() {
    var style = IconPack.atomaMono.style
    style.autoTrim = false
    let rep = render(style)
    for (fx, fy) in [(0.5, 0.5), (0.45, 0.55), (0.55, 0.45)] {
        let c = pixel(rep, fx, fy)
        let spread = max(c.redComponent, c.greenComponent, c.blueComponent) - min(c.redComponent, c.greenComponent, c.blueComponent)
        #expect(spread < 0.04, "pixel (\(fx), \(fy)) is tinted: \(c)")
    }
}

@Test("Without monochrome the red icon keeps its colour")
func colourSurvives() {
    var style = RoundIconStyle(shape: .circle, shadowIntensity: 0)
    style.autoTrim = false
    let c = pixel(render(style), 0.5, 0.5)
    #expect(c.redComponent - c.blueComponent > 0.4)
}
