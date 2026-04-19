// Export functions, strings, and globals into reccmp.
//@author some100
//@category TH07
//@keybinding
//@menupath
//@toolbar
//@runtime Java

import ghidra.app.script.GhidraScript;
import ghidra.pcode.floatformat.BigFloat;
import ghidra.program.model.address.*;
import ghidra.program.model.block.*;
import ghidra.program.model.data.*;
import ghidra.program.model.data.ISF.*;
import ghidra.program.model.gclass.*;
import ghidra.program.model.lang.*;
import ghidra.program.model.lang.protorules.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.pcode.*;
import ghidra.program.model.reloc.*;
import ghidra.program.model.scalar.*;
import ghidra.program.model.sourcemap.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.util.*;
import java.io.File;
import java.io.PrintWriter;
import java.math.BigDecimal;

public class ExportReccmpCsv extends GhidraScript {

    private String formatValue(double val) {
        String s = new BigDecimal(Double.toString(val)).toPlainString();

        if (!s.contains(".")) {
            s += ".0";
        }
        return s;
    }

    private String cleanFuncName(String name) {
        if (name.startsWith("_")) name = name.substring(1);
        if (
            name.contains("`scalar_deleting_destructor'") ||
            name.contains("`vector_deleting_destructor'") ||
            name.contains("`eh_vector_constructor_iterator'") ||
            name.contains("`eh_vector_destructor_iterator'") ||
            name.equals("operator_new") ||
            name.equals("operator_delete")
        ) name = name.replace("_", " ");
        return name;
    }

    private String escapeControlChars(String in) {
        return in
            .replace("\\", "\\\\")
            .replace("\n", "\\n")
            .replace("\r", "\\r")
            .replace("\t", "\\t")
            .replace("\"", "\\\"")
            .replace("\'", "\\\'");
    }

    private String exportFuncs(Program p) {
        StringBuilder csvFile = new StringBuilder();
        csvFile.append("address|name|type\n");
        FunctionIterator funcs = p.getListing().getFunctions(true);
        while (funcs.hasNext()) {
            Function func = funcs.next();
            if (func.isExternal()) continue;
            String address = func.getEntryPoint().toString(false);
            String name = cleanFuncName(func.getName(true));
            String type =
                func.getEntryPoint().getOffset() <= 0x0045ffc0
                    ? "function"
                    : "library";
            csvFile
                .append(address)
                .append("|")
                .append(name)
                .append("|")
                .append(type)
                .append("\n");
        }
        return csvFile.toString();
    }

    private String exportStrings(Program p) {
        StringBuilder csvFile = new StringBuilder();
        csvFile.append("address|text|type\n");
        DataIterator datas = p.getListing().getDefinedData(true);
        while (datas.hasNext()) {
            Data data = datas.next();
            if (
                !(data.getDataType() instanceof TerminatedStringDataType)
            ) continue;
            String address = data.getAddressString(false, false);
            String text = escapeControlChars(data.getValue().toString());
            String type = "string";
            csvFile
                .append(address)
                .append("|")
                .append(text)
                .append("|")
                .append(type)
                .append("\n");
        }
        return csvFile.toString();
    }

    private String exportFloats(Program p) {
        StringBuilder csvFile = new StringBuilder();
        csvFile.append("address|name|type\n");
        DataIterator datas = p.getListing().getDefinedData(true);
        while (datas.hasNext()) {
            Data data = datas.next();
            DataType dt = data.getBaseDataType();
            if (
                !(dt instanceof FloatDataType || dt instanceof DoubleDataType)
            ) continue;
            String address = data.getAddressString(false, false);
            BigFloat val = (BigFloat) data.getValue();

            String name;
            if (dt instanceof FloatDataType) name = formatValue(
                (double) val.toBigDecimal().floatValue()
            );
            else name = formatValue(val.toBigDecimal().doubleValue());
            String type = "float";
            csvFile
                .append(address)
                .append("|")
                .append(name)
                .append("|")
                .append(type)
                .append("\n");
        }
        return csvFile.toString();
    }

    private String exportGlobals(Program p) {
        StringBuilder csvFile = new StringBuilder();
        csvFile.append("address|name|type\n");

        SymbolIterator symbols = p.getSymbolTable().getAllSymbols(true);
        while (symbols.hasNext()) {
            Symbol symbol = symbols.next();
            if (symbol.getSymbolType() != SymbolType.LABEL) continue;
            if (symbol.getSource() != SourceType.USER_DEFINED) continue;
            if (
                p.getListing().getDefinedDataAt(symbol.getAddress()) == null
            ) continue;
            String address = symbol.getAddress().toString(false, false);

            String name = symbol.getName(true);
            String type = "global";
            if (name.contains("vftable")) {
                name = symbol.getParentNamespace().getName();
                type = "vtable";
            }
            csvFile
                .append(address)
                .append("|")
                .append(name)
                .append("|")
                .append(type)
                .append("\n");
        }
        return csvFile.toString();
    }

    public void run() throws Exception {
        File outDir = askDirectory("Output folder", "Open");
        Program p = currentProgram;
        PrintWriter funcWriter = new PrintWriter(
            new File(outDir, "funcs.csv"),
            "Shift_JIS"
        );
        funcWriter.write(exportFuncs(p));
        PrintWriter strWriter = new PrintWriter(
            new File(outDir, "strings.csv"),
            "Shift_JIS"
        );
        strWriter.write(exportStrings(p));
        PrintWriter floatWriter = new PrintWriter(
            new File(outDir, "floats.csv"),
            "Shift_JIS"
        );
        floatWriter.write(exportFloats(p));
        PrintWriter globalWriter = new PrintWriter(
            new File(outDir, "globals.csv"),
            "Shift_JIS"
        );
        globalWriter.write(exportGlobals(p));

        funcWriter.close();
        strWriter.close();
        floatWriter.close();
        globalWriter.close();
    }
}
