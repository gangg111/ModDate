from PIL import Image, ImageDraw
def draw(n):
    s=4*n; im=Image.new("RGBA",(s,s),(0,0,0,0)); d=ImageDraw.Draw(im)
    # kartka pliku
    d.rounded_rectangle([s*.14,s*.06,s*.72,s*.90],radius=s*.06,fill=(245,247,250),outline=(70,90,120),width=max(1,s//40))
    for i in range(3): d.rectangle([s*.24,s*(.24+i*.12),s*.60,s*(.28+i*.12)],fill=(150,165,185))
    # zegar
    c=(s*.66,s*.66); r=s*.30
    d.ellipse([c[0]-r,c[1]-r,c[0]+r,c[1]+r],fill=(0,110,200),outline=(255,255,255),width=max(1,s//28))
    w=max(2,s//22)
    d.line([c,(c[0],c[1]-r*.62)],fill="white",width=w); d.line([c,(c[0]+r*.48,c[1])],fill="white",width=w)
    return im.resize((n,n),Image.LANCZOS)
imgs=[draw(n) for n in (256,64,48,32,24,16)]
imgs[0].save(__import__("os").path.join(__import__("os").path.dirname(__file__),"app.ico"),sizes=[(i.width,i.height) for i in imgs],append_images=imgs[1:])
