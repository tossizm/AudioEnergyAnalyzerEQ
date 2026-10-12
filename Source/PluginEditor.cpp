#include "PluginEditor.h"
#include <cmath>

ClubAudioAnalyzerEQAudioProcessorEditor::ClubAudioAnalyzerEQAudioProcessorEditor(ClubAudioAnalyzerEQAudioProcessor& p)
    :AudioProcessorEditor(&p),processor(p)
{
    setSize(1040,650);
    title.setText("ClubAudioAnalyzer EQ",juce::dontSendNotification);
    title.setFont(juce::Font(24.0f,juce::Font::bold)); title.setColour(juce::Label::textColourId,juce::Colours::white); addAndMakeVisible(title);
    status.setText("Reference: "+processor.getReferenceName(),juce::dontSendNotification);
    status.setColour(juce::Label::textColourId,juce::Colours::lightblue); addAndMakeVisible(status);
    curveTitle.setText("Frequency Response",juce::dontSendNotification); curveTitle.setColour(juce::Label::textColourId,juce::Colours::white); addAndMakeVisible(curveTitle);
    addAndMakeVisible(loadJsonButton); loadJsonButton.onClick=[this]{chooseFile(LoadMode::json);};
    addAndMakeVisible(loadTxtButton); loadTxtButton.onClick=[this]{chooseFile(LoadMode::txt);};
    addAndMakeVisible(resetButton); resetButton.onClick=[this]{for(int i=0;i<10;++i)if(auto* p=processor.parameters.getParameter("band"+juce::String(i)))p->setValueNotifyingHost(p->convertTo0to1(0.0f)); repaint();};
    const juce::String names[]={"20–60 Hz","60–120 Hz","120–250 Hz","250–500 Hz","500–1000 Hz","1000–2000 Hz","2000–5000 Hz","5000–10000 Hz","10000–16000 Hz","16000–20000 Hz"};
    for(int i=0;i<10;++i){
        labels[(size_t)i].setText(names[i],juce::dontSendNotification); labels[(size_t)i].setJustificationType(juce::Justification::centred);
        labels[(size_t)i].setColour(juce::Label::textColourId,juce::Colours::white); addAndMakeVisible(labels[(size_t)i]);
        auto& s=sliders[(size_t)i]; s.setSliderStyle(juce::Slider::LinearVertical); s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,70,22);
        s.setRange(-12.0,12.0,0.01); s.setTextValueSuffix(" dB"); s.onValueChange=[this]{repaint();}; addAndMakeVisible(s);
        attachments[(size_t)i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,"band"+juce::String(i),s);
    }
}
void ClubAudioAnalyzerEQAudioProcessorEditor::chooseFile(LoadMode mode)
{
    chooser.launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [this,mode](const juce::FileChooser& fc){
            const auto file=fc.getResult(); if(!file.existsAsFile())return;
            juce::String msg; const bool ok=mode==LoadMode::json?processor.loadReferenceJson(file,msg):processor.loadAnalysisTxt(file,msg);
            status.setText((ok?"OK: ":"Error: ")+msg,juce::dontSendNotification); repaint();
        });
}
void ClubAudioAnalyzerEQAudioProcessorEditor::drawResponse(juce::Graphics& g,juce::Rectangle<float> r)
{
    g.setColour(juce::Colour::fromRGB(13,17,29)); g.fillRoundedRectangle(r,5.0f);
    auto plot=r.reduced(42.0f,18.0f);
    const float minDb=-18.0f,maxDb=18.0f;
    auto yFor=[&](float db){return juce::jmap(juce::jlimit(minDb,maxDb,db),maxDb,minDb,plot.getY(),plot.getBottom());};
    auto xFor=[&](float hz){return juce::jmap(std::log10(juce::jlimit(20.0f,20000.0f,hz)),std::log10(20.0f),std::log10(20000.0f),plot.getX(),plot.getRight());};
    g.setFont(11.0f);
    for(int db=-18;db<=18;db+=6){
        float y=yFor((float)db); g.setColour(db==0?juce::Colours::grey.withAlpha(0.8f):juce::Colours::white.withAlpha(0.12f));
        g.drawHorizontalLine((int)y,plot.getX(),plot.getRight()); g.setColour(juce::Colours::lightgrey); g.drawText(juce::String(db), (int)r.getX()+4,(int)y-7,32,14,juce::Justification::centredRight);
    }
    const int hzTicks[]={20,50,100,200,500,1000,2000,5000,10000,20000};
    for(int hz:hzTicks){float x=xFor((float)hz);g.setColour(juce::Colours::white.withAlpha(0.10f));g.drawVerticalLine((int)x,plot.getY(),plot.getBottom());g.setColour(juce::Colours::lightgrey);g.drawText(hz>=1000?juce::String(hz/1000)+"k":juce::String(hz),(int)x-20,(int)plot.getBottom()+4,40,14,juce::Justification::centred);}
    juce::Path path;
    for(int i=0;i<=420;++i){float hz=(float)(20.0*std::pow(1000.0,(double)i/420.0));float x=xFor(hz),y=yFor(processor.getResponseDb(hz));if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
    g.setColour(juce::Colour::fromRGB(72,205,255));g.strokePath(path,juce::PathStrokeType(2.5f));
    for(int i=0;i<10;++i){float x=xFor(ClubAudioAnalyzerEQAudioProcessor::centresHz[(size_t)i]);float y=yFor(processor.getResponseDb(ClubAudioAnalyzerEQAudioProcessor::centresHz[(size_t)i]));
        g.setColour(juce::Colour::fromRGB(255,191,71));g.fillEllipse(x-4.5f,y-4.5f,9.0f,9.0f);
        g.setColour(juce::Colours::white);g.drawText(juce::String(i+1),(int)x-8,(int)y-19,16,14,juce::Justification::centred);
    }
}
void ClubAudioAnalyzerEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(1,0,102));g.setColour(juce::Colours::white.withAlpha(0.22f));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(5.0f),8.0f,1.0f);
    drawResponse(g,juce::Rectangle<float>(16.0f,92.0f,(float)getWidth()-32.0f,255.0f));
}
void ClubAudioAnalyzerEQAudioProcessorEditor::resized()
{
    auto area=getLocalBounds().reduced(16);auto top=area.removeFromTop(36);title.setBounds(top.removeFromLeft(300));
    resetButton.setBounds(top.removeFromRight(100));loadTxtButton.setBounds(top.removeFromRight(180).reduced(4,0));loadJsonButton.setBounds(top.removeFromRight(180).reduced(4,0));
    status.setBounds(area.removeFromTop(26));curveTitle.setBounds(20,68,300,20);
    area.removeFromTop(290);
    const int colW=area.getWidth()/10;
    for(int i=0;i<10;++i){auto col=area.removeFromLeft(i==9?area.getWidth():colW);labels[(size_t)i].setBounds(col.removeFromTop(42));sliders[(size_t)i].setBounds(col.reduced(3,0));}
}
